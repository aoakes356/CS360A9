#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include "mftp.h"
#include <sys/wait.h>

// Client Code.

#define CUST_PORT 49999 
// Returns a connected socket.
int serverConnect(char* ip);
// Process the arguments. This thing uses malloc >:O
// Reads from stdin.
char** getArgs(int* argc);
int processArgs(char** arguments, int socket, int args, char* ip);
int getWord(char* word, int size);
int moreify(char** arguments);
int moreifyfd(int fd2);

int main(int argc, char** argv){
    char* ip;
    int socket;
    // Check the input.
    if(argc > 1){
        ip = argv[1];
    }else{
        printf("No ip given.\n");
        return 1;
    }
    // Get a connected socket.
    socket = serverConnect(ip);
    if(socket < 0){
        return errorHandler("Failed to connect to the server");
    }
    printf("Successfully connected to the server!\n");
    // Send a message.
    int argCount, res;
    char** temp;
    while(1){
        temp = getArgs(&argCount);
        if((res = processArgs(temp, socket, argCount, ip)) || res < 0){ // If true, exit was recieved.
            break;
        } 
        //Do stuff with args here.
        for(int i = 0; i < argCount; i++){
            free(temp[i]);
        }
        free(temp);
    }
    printf("Exiting to get message.\n");
    if(getMessage(socket) < 0) { return errorHandler("Failed to recieve message");}
    // Close the connection.
    if(closeConnection(socket) < 0){return errorHandler("Failed to close connection.");}
}

char* getwordsocket(int socket){
    int rd = 1, size = 10, count = 0;
    char* clientIn = malloc(sizeof(char)*size);
    char* clsave = clientIn;
    while(rd){
        printf("Waiting for the server to respond.....\n");
        if((rd = read(socket,clientIn,1)) < 0){ 
            errorHandler("Read error in connectionHandler");
            return NULL;
        }
        count++;
        if(count >= size && rd){
            size *= 2;
            clsave = realloc(clsave,size*sizeof(char));
        }
        if(*clientIn == '\n' || *clientIn == EOF){
            *clientIn = '\0';
            break;
        }
        printf("read in: %c\n",*clientIn);
        clientIn++;
    }
    if(count <= 1){
        free(clsave);
        return NULL;
    }
    return clsave;

}


int errorHandler(char* message){ // Just a convenient method to have.
    fprintf(stderr,"CSTMERR %s: %s\n", message, strerror(errno));
    printf("%s: %s\n", message, strerror(errno));
    return -1;
}
int printD(){
    int res = fork();
    if(res < 0){return errorHandler("Failed to fork in printD");}
    if(!res){
        // Child
        char* args[5] = {"ls","-l",NULL};
        execvp(args[0],(char**)args);
        exit(1);
    }else{
        wait(NULL); // I thought this wouldn't be needed, but it helps even though the child is vaporized by exec.
        return 0;
    }
}

/* Create the socket for the client side to connect to the server
 * Then connect.
 * */
int serverConnect(char* ip){

    // Will store the server address.
    struct sockaddr_in serverAddr;
    // Set all values of serverAddr to 0.
    // Really not sure why this is necessary, but it is done in the slides.
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(CUST_PORT);
    // Used to store the result of gethostby name 
    // will contain the server address in h_name
    struct hostent* hostEntry;
    hostEntry = gethostbyname(ip);
    herror("attempted to get host address");
    int sock = socket(PF_INET,SOCK_STREAM, IPPROTO_TCP);
    // Copy the resolved host name from hostEntry into the sockaddr_in struct.
    struct in_addr **pptr = (struct in_addr **) hostEntry->h_addr_list; // From slides
    memcpy(&serverAddr.sin_addr, *pptr, sizeof(struct in_addr));

    if(sock < 0) {return errorHandler("Failed to create the socket.");}
    // Attempt to connect to the server
    // cast serverAddr to the generic socket address struct.
    int stat = connect(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if(stat < 0){return errorHandler("Failed to connect to the server. SERVERCONNECT");}
    return sock;
}
int serverConnectPort(char* ip, int port){

    // Will store the server address.
    struct sockaddr_in serverAddr;
    // Set all values of serverAddr to 0.
    // Really not sure why this is necessary, but it is done in the slides.
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = port;
    // Used to store the result of gethostby name 
    // will contain the server address in h_name
    struct hostent* hostEntry;
    hostEntry = gethostbyname(ip);
    herror("attempted to get host address");
    int sock = socket(PF_INET,SOCK_STREAM, IPPROTO_TCP);
    // Copy the resolved host name from hostEntry into the sockaddr_in struct.
    struct in_addr **pptr = (struct in_addr **) hostEntry->h_addr_list; // From slides
    memcpy(&serverAddr.sin_addr, *pptr, sizeof(struct in_addr));

    if(sock < 0) {return errorHandler("Failed to create the socket.");}
    // Attempt to connect to the server
    // cast serverAddr to the generic socket address struct.
    int stat = connect(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if(stat < 0){return errorHandler("Failed to connect to the server. SERVERCONNECTPORT");}
    return sock;
}

int message(char* msg, int socket){
    int len = strlen(msg);
    //if(send(socket,msg,(len),0) != len){return errorHandler("Didn't send expected number of bytes.");}
    int res = write(socket,msg,len+1);
    if(res != len+1){return errorHandler("Didn't write the entire message?!??!?!?!?!??!??!!!");}
    return 0;

}

// Get a message from the server.
int getMessage(int socket){
    char buffer[256];
    int len;
    if((len=recv(socket,buffer, 256,0)) < 0){return errorHandler("Failed to recieve message.");}
    if(len < 256){
        buffer[len] = '\0';
    }
    printf("Recieved: %s\n",buffer);
    return 0;
}
int closeConnection(int socket){
    if(close(socket) < 0){return errorHandler("Failed to close the file descriptor.");}
    return 0;

}
// Get the arguments from le user.
char** getArgs(int* argc){
    int size = 3, count = 0, wordsize = 10, c;
    char** argument_list = malloc(sizeof(char*)*size);
    printf("> ");
    while((c = getWord(argument_list[count++]=malloc(sizeof(char)*wordsize),wordsize)) != EOF && c != '\n'){
        if(count >= size){
            size *= 2;
            argument_list = realloc(argument_list, size*sizeof(char*));
        }
    }
    argument_list[count++] = NULL;
    *argc = count-1;
    return argument_list;
}

int getWord(char* word, int size){
    int count = 0;
    char c;
    while((c = getchar()) != EOF && (!isspace(c))){
        word[count++] = c;
        if(count >= size){
            size *= 2;
            word = realloc(word,sizeof(char)*size);
        }
    }
    word[count] = '\0';
    return c;
}

int getPort(char* serverresponse){
    // return -1 for error.
    int port;
    if(!sscanf(serverresponse,"A%d",&port)){ return errorHandler("No port given in response, Error.");}
    return port;
}



int processArgs(char** arguments, int socket, int argc, char* ip){
    char* argbuffer;
    char* serverresponse;
    int size;
    if(argc > 1){
        size = strlen(arguments[1])+3;
    }else{
        size = 3;
    }
    argbuffer = malloc(sizeof(char)*size);
    if(strncmp(arguments[0],"exit",5) == 0){
        free(argbuffer);
        return 1;   // Exit code is 1.
    }else if(strncmp(arguments[0],"cd",3) == 0){
        // Change directory
        if(chdir(arguments[1]) < 0){return errorHandler("Failed to change directory locally.");}
    }else if(strncmp(arguments[0],"ls",3) == 0){
        // Print out stuff in cwd ls -l.
        char* args[3] = {"ls","-l",NULL};
        if(moreify(args) < 0){ return errorHandler("Failed to ls -l");}
    }else if(strncmp(arguments[0],"rcd",4) == 0){
        // Change current working directory of current connection to server.
        argbuffer[0] = 'C';argbuffer[2] = '\0';
        if(argc > 1){
            strncat(argbuffer,arguments[1],size-3);
            argbuffer[size-4] = '\n';
        }
        if(message(argbuffer,socket) < 0){return errorHandler("Failed to message in arg handler.");}
    }else if(strncmp(arguments[0],"rls",4) == 0){
        // Print stuff from cwd of server child into client console.
        // Send D to establish data connection, then send L to have it send back ls -l output, moreify it.
        if(message("D\n",socket) < 0){return errorHandler("Failed to message data signal in arg handler.");}
        printf("Sent a message to the server\n");
        // Establish the connection for the output to come back on.
        serverresponse = getwordsocket(socket);
        if(serverresponse == NULL){ return errorHandler("Failed to get a response from the server.");}
        printf("--ServerResponse %s\n",serverresponse);
        int dataPort = getPort(serverresponse);
        if(dataPort < 0){return errorHandler("Failed to get a valid port number from the server response.");}
        printf("Le port: %i\n",dataPort);
        if(serverConnectPort(ip, dataPort) < 0){ return errorHandler("Failed to connect to the given IP and port");}
        // Make a connection to that port on this side.
        argbuffer[0] = 'L';argbuffer[1] = '\0';
        if(argc > 1){
            strncat(argbuffer,arguments[1],size-3);
            argbuffer[size-4] = '\n';
        }
        if(message(argbuffer,socket) < 0){return errorHandler("Failed to message in arg handler.");}
        //pipe the fd into more :D almost done.
        if(moreifyfd(dataPort) < 0){return errorHandler("Failed to read from the dataport.");}
    }else if(strncmp(arguments[0],"get",4) == 0){
        // Get a file from the server
        if(message("D\n",socket) < 0){return errorHandler("Failed to message data signal in arg handler.");}
        //Establish the connection.
        argbuffer[0] = 'G';argbuffer[2] = '\0';
        if(argc > 1){
            strncat(argbuffer,arguments[1],size-3);
            argbuffer[size-4] = '\n';
        }
        if(message(argbuffer,socket) < 0){return errorHandler("Failed to message in arg handler.");}
    }else if(strncmp(arguments[0],"show",5) == 0){
        // Show the contents of pathname on server to client.
        // Like get but instead of writing to the hard drive, print to stdout pipe into more.
        // Get a file from the server
        if(message("D\n",socket) < 0){return errorHandler("Failed to message data signal in arg handler.");}
        //Establish the connection.
        argbuffer[0] = 'G';argbuffer[2] = '\0';
        if(argc > 1){
            strncat(argbuffer,arguments[1],size-3);
            argbuffer[size-4] = '\n';
        }
        if(message(argbuffer,socket) < 0){return errorHandler("Failed to message in arg handler.");}
    }else if(strncmp(arguments[0],"put",4) == 0){
        // put a file from the client onto the server.
        if(message("D\n",socket) < 0){return errorHandler("Failed to message data signal in arg handler.");}
        //Establish the connection.
        argbuffer[0] = 'P';argbuffer[2] = '\0';
        if(argc > 1){
            strncat(argbuffer,arguments[1],size-3);
            argbuffer[size-4] = '\n';
        }
        if(message(argbuffer,socket) < 0){return errorHandler("Failed to message in arg handler.");}
    }
    free(argbuffer);
    return 0;
}

int moreify(char** arguments){
    int fd[2];
    char* args[5] = {"more","-20",NULL};
    if(pipe(fd) < 0){ return errorHandler("Failed to create pipe.");}
    int res = fork();
    if(res < 0){return errorHandler("Failed to fork.");}
    if(!res){
        // Child.
        dup2(fd[1],1);
        if(close(fd[0]) < 0) fprintf(stderr,"%s\n",strerror(errno));
        execvp(arguments[0],arguments);
        return errorHandler("Failed to execute more!");

    }else{
        // Parent.
        wait(NULL);
        dup2(fd[0],0);
        if(close(fd[1]) < 0) fprintf(stderr,"%s\n",strerror(errno));
        execvp(args[0],args);
        return errorHandler("Failed to execute crap.");
    }
}

int moreifyfd(int fd2){
    int fd[2];
    char* args[5] = {"more","-20",NULL};
    if(pipe(fd) < 0){ return errorHandler("Failed to create pipe.");}
    int res = fork();
    char c;
    if(res < 0){return errorHandler("Failed to fork.");}
    if(!res){
        // Child.
        dup2(fd[1],1);
        if(close(fd[0]) < 0) fprintf(stderr,"%s\n",strerror(errno));
        int count = 1;
        while(count){
            if(count = read(fd2,&c,1) < 0){return errorHandler("Failed to read stuff.");}
            if(count){
                if(write(1,&c,1) < 0){return errorHandler("Failed to write stuff.");}
            }
        }
    }else{
        // Parent.
        wait(NULL);
        dup2(fd[0],0);
        if(close(fd[1]) < 0) fprintf(stderr,"%s\n",strerror(errno));
        execvp(args[0],args);
        return errorHandler("Failed to execute more!");
    }

}

