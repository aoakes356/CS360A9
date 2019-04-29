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
#include <fcntl.h>

// Client Code.

#define CUST_PORT 49999 
// Returns a connected socket.
int serverConnect(char* ip);
// Process the arguments. This thing uses malloc >:O
// Reads from stdin.
char** getArgs(int* argc);
int processArgs(char** arguments, int socket, int args, char* ip);
int getWord(char*** argument_list, int pos, int size);
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
        if((res = processArgs(temp, socket, argCount, ip)) && res > 0){ // If true, exit was recieved.
            printf("The client is exiting");
            break;
        } 
        for(int i = 0; i < argCount; i++){
            free(temp[i]);
        }
        free(temp);
    }
    for(int i = 0; i < argCount; i++){
        free(temp[i]);
    }
    free(temp);
    // Close the connection.
    if(closeConnection(socket) < 0){return errorHandler("Failed to close connection.");}
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
    serverAddr.sin_port = htons(port);
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


// Close with built in error checking.
int closeConnection(int socket){
    if(close(socket) < 0){return errorHandler("Failed to close the file descriptor.");}
    return 0;

}
// Get the arguments from le user.
char** getArgs(int* argc){
    int size = 3, count = 0, wordsize = 10, c;
    char** argument_list = malloc(sizeof(char*)*size);
    printf("> ");
    while((c = getWord(&argument_list,count,wordsize)) != EOF && c != '\n'){
        count += 1;
        if(count >= size){
            size *= 2;
            argument_list = realloc(argument_list, size*sizeof(char*));
        }
    }
    argument_list[count+1] = NULL;
    *argc = count+1;
    return argument_list;
}

int getWord(char*** argument_list, int pos, int size){
    int count = 0;
    char* word = malloc(sizeof(char)*size);
    char c;
    while((c = getchar()) != EOF && (!isspace(c))){
        word[count++] = c;
        if(count >= size){
            size *= 2;
            word = realloc(word,sizeof(char)*size);
        }
    }
    word[count] = '\0';
    printf("%s Aquired word size:1 %i\n",word,size);
    (*argument_list)[pos] = word;
    printf("%s Aquired word size:2 %i\n",word,size);
    printf("%s Aquired word size:3 %i\n",(*argument_list)[pos],size);
    return c;
}
// Get the string from the server response.
int getPort(char* serverresponse){
    // return -1 for error.
    int port;
    if(!sscanf(serverresponse,"A%d",&port)){ return errorHandler("No port given in response, Error.");}
    return port;
}
// Establish a dataconnection with the server, return a file descriptor or -1 for error.
int getDataConnection(int socket, char* ip){
    int datafd;
    if(message("D\n",socket) < 0){
        return errorHandler("Failed to message data signal in arg handler.");
    }
    printf("Sent a message to the server\n");
    // Establish the connection for the output to come back on.
    char* serverresponse = getwordsocket(socket);
    if(serverresponse == NULL){ 
        return errorHandler("Failed to get a response from the server.");
    }
    printf("--ServerResponse %s\n",serverresponse);
    int dataPort = getPort(serverresponse); // Get the port from t
    if(dataPort < 0){
        return errorHandler("Failed to get a valid port number from the server response.");
    }
    printf("Le port: %i\n",dataPort);
    if((datafd=serverConnectPort(ip, dataPort)) < 0){ 
        return errorHandler("Failed to connect to the given IP and port");
    }
    printf("connected to le port on the server.");
    return datafd;
}

int processArgs(char** arguments, int socket, int argc, char* ip){
    char* argbuffer;
    char* serverresponse;
    int size;
    if(argc > 1){
        size = strlen(arguments[1])+3;
        printf("-----------------%s size %i\n",arguments[1],size);

    }else{
        size = 3;
        printf("-----------------%s size %i\n",arguments[1],size);
    }
    argbuffer = malloc(sizeof(char)*size);
    if(strncmp(arguments[0],"exit",5) == 0){
        free(argbuffer);
        if(message("Q\n",socket) < 0){errorHandler("Failed to end server connection.");}
        return 1;   // Exit code is 1.
    }else if(strncmp(arguments[0],"cd",3) == 0){
        // Change directory
        if(chdir(arguments[1]) < 0){
            free(argbuffer);
            return errorHandler("Failed to change directory locally.");
        }
    }else if(strncmp(arguments[0],"ls",3) == 0){
        // Print out stuff in cwd ls -l.
        char* args[3] = {"ls","-l",NULL};
        if(moreify(args) < 0){ 
            free(argbuffer);
            return errorHandler("Failed to ls -l");
        }
    }else if(strncmp(arguments[0],"rcd",3) == 0){
        printf("aeareareara aAAA Entering rcd\n");
        // Change current working directory of current connection to server.
        argbuffer[0] = 'C';argbuffer[2] = '\0';
        if(argc > 1){
            sprintf(argbuffer,"C%s\n",arguments[1]);
            printf("!!!!!!!!!!1ARguments to be sent---------------> %s\n",argbuffer);
        }else{
            argbuffer[1] = '\n';
            argbuffer[2] = '\0';
        }

        if(message(argbuffer,socket) < 0){
            free(argbuffer);
            return errorHandler("Failed to message in arg handler.");
        }
        serverresponse = getwordsocket(socket);
        if(serverresponse[0] != 'A'){
            errorHandler(serverresponse);
        }
        free(serverresponse);
    }else if(strncmp(arguments[0],"rls",4) == 0){
        // Print stuff from cwd of server child into client console.
        // Send D to establish data connection, then send L to have it send back ls -l output, moreify it.
        int datafd = getDataConnection(socket,ip);
        if(datafd < 0){return errorHandler("Failed to get data connection.");}
        // Make a connection to that port on this side.
        argbuffer[0] = 'L';argbuffer[1] = '\0';
        if(argc > 1){
            strncat(argbuffer,arguments[1],size-3);
            argbuffer[size-4] = '\n';
        }
        else{
            argbuffer[1] = '\n';
            argbuffer[2] = '\0';
        }
        printf("Constructed command: %s",argbuffer);
        if(message(argbuffer,socket) < 0){
            free(argbuffer);
            return errorHandler("Failed to message in arg handler.");
        }
        printf("Sent command to the parent. Waiting for acceptance.");
        //free(serverresponse);
        serverresponse = getwordsocket(socket);
        // recieve validation from the server. 
        if(serverresponse == NULL || strncmp(serverresponse,"A",2)){
            free(argbuffer);
            free(serverresponse);
            return errorHandler("Server failed to ls, non-accept response.");
        }
        free(serverresponse);
        // Thank you server I feel much better now.
        // pipe the fd into more :D almost done.
        if(moreifyfd(datafd) < 0){
            free(argbuffer);
            return errorHandler("Failed to read from the dataport.");
        }
        if(close(datafd) < 0){
            free(argbuffer);
            return errorHandler("Failed to close datafd.");
        }
    }else if(strncmp(arguments[0],"get",3) == 0){
        // Get a file from the server
        printf("Getting a file from the server.\n");
        int datafd = getDataConnection(socket,ip);
        if(datafd < 0){return errorHandler("Failed to get data connection.");}
        //Establish the connection.
        argbuffer[0] = 'G';argbuffer[2] = '\0';
        if(argc > 1){
            sprintf(argbuffer,"G%s\n",arguments[1]);
        }else{
            argbuffer[1] = '\n';
            argbuffer[2] = '\0';
        }

        if(message(argbuffer,socket) < 0){
            free(argbuffer);
            return errorHandler("Failed to message in arg handler.");
        }
        serverresponse = getwordsocket(socket);
        if(serverresponse[0] != 'A'){return errorHandler("Failed to get file, non-accept response.");}
        printf("File is ready to be read in\n");
        int new;
        if((new = createFile(&(argbuffer[1]))) < 0){return errorHandler("Unable to create file.");}
        if(copyFile(datafd,new) < 0){return errorHandler("Unable to copy data to new file.");}
        if(close(datafd) < 0){
            free(argbuffer);
            return errorHandler("Failed to close datafd.");
        }
    }else if(strncmp(arguments[0],"show",5) == 0){
        // Show the contents of pathname on server to client.
        // Like get but instead of writing to the hard drive, print to stdout pipe into more.
        // Get a file from the server
        // Get a file from the server
        printf("Getting a file from teh server.\n");
        int datafd = getDataConnection(socket,ip);
        if(datafd < 0){return errorHandler("Failed to get data connection.");}
        //Establish the connection.
        argbuffer[0] = 'G';argbuffer[2] = '\0';
        if(argc > 1){
            sprintf(argbuffer,"G%s\n",arguments[1]);
        }else{
            argbuffer[1] = '\n';
            argbuffer[2] = '\0';
        }

        if(message(argbuffer,socket) < 0){
            free(argbuffer);
            return errorHandler("Failed to message in arg handler.");
        }
        serverresponse = getwordsocket(socket);
        if(serverresponse[0] != 'A'){return errorHandler("Failed to get file, non-accept response.");}
        printf("File is ready to be read in\n");
        int new;
        if((new = createFile(&(argbuffer[1]))) < 0){return errorHandler("Unable to create file.");}
        if(moreifyfd(datafd) < 0){return errorHandler("Unable to copy data to new file.");}
        if(close(datafd) < 0){
            free(argbuffer);
            return errorHandler("Failed to close datafd.");
        }
    }else if(strncmp(arguments[0],"put",3) == 0){
        // put a file from the client onto the server.
        int datafd = getDataConnection(socket,ip);
        if(datafd < 0){return errorHandler("Failed to get data connection.");}
        //Establish the connection.
        argbuffer[0] = 'P';argbuffer[2] = '\0';
        if(argc > 1){
            sprintf(argbuffer,"P%s\n",getFileName(arguments[1]));
        }else{
            argbuffer[1] = '\n';
            argbuffer[2] = '\0';
        }

        if(message(argbuffer,socket) < 0){
            free(argbuffer);
            return errorHandler("Failed to message in arg handler.");
        }
        serverresponse = getwordsocket(socket);
        if(serverresponse[0] != 'A'){return errorHandler("Failed, non-accept response.");}
        int fd = open(arguments[1],O_RDONLY);
        if(fd < 0){ return errorHandler("Failed to open file.");}
        if(copyFile(fd, datafd) < 0){ return errorHandler("Failed to write to the data connection.");} 
    }
    free(argbuffer);
    return 0;
}

int moreify(char** arguments){
    int outer = fork();
    if(outer < 0){return errorHandler("Failed to fork.");}
    if(!outer){
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
            return errorHandler("Failed to execute crap");

        }else{
            // Parent.
            wait(NULL);
            dup2(fd[0],0);
            if(close(fd[1]) < 0) fprintf(stderr,"%s\n",strerror(errno));
            execvp(args[0],args);
            return errorHandler("Failed to execute more!");
        }
    }else{
        wait(NULL);
        return 0;
    }
}

int moreifyfd(int fd2){
    int outer = fork();
    char* args[5] = {"more","-20",NULL};
    if(outer < 0){return errorHandler("Failed to fork.");}
    if(!outer){    
        dup2(fd2,0);
        execvp(args[0],args);
        return errorHandler("Failed to execute more!");
	return 0;
    }else{
        // Parent.
        wait(NULL);
    }
    return 0;
}

