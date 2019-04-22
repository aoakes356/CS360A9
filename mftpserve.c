#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>
#include "mftp.h"
// Server Code.

#define CUST_PORT 49999
// Create the socket and set it to listen for connections.
int startServer();
// Forks and deals with a connection.
int connectionHandler(int socket);
// Get the name of the connection host.
int getHost(struct sockaddr_in client);

int main(){

    int socket = startServer();
    struct sockaddr_in client;
    int length = sizeof(struct sockaddr_in);
    int connectionfd;
    // wait for connection, fork each time there is a connection and deal with it.
    while(1){
        if((connectionfd = accept(socket,(struct sockaddr*)&client,&length)) < 0){
            return errorHandler("Failed to accept a connection.");
        }
        if(getHost(client) < 0){errorHandler("Failed to get hostname");}
        connectionHandler(connectionfd);
	    close(connectionfd);
    }
}

int errorHandler(char* message){ // Just a convenient method to have.
    fprintf(stderr,"CSTMERR %s: %s\n", message, strerror(errno));
    printf("%s: %s\n", message, strerror(errno));
    return -1;
}

int startServer(){
    // Create the socket
    int sock = socket(PF_INET,SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in inSockAddr;
    // Again not sure why this is necessary.
    memset(&inSockAddr, 0, sizeof(inSockAddr));
    inSockAddr.sin_family = AF_INET;
    inSockAddr.sin_port = htons(CUST_PORT);
    inSockAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any address.
    if(sock < 0){return errorHandler("Failed to create socket");}
    // Bind it to the port
    if(bind(sock,(struct sockaddr*)&inSockAddr, sizeof(inSockAddr)) < 0){
        return errorHandler("Failed to bind the socket.");
    }
    // set to listen for connections.
    if(listen(sock,4) < 0){ return errorHandler("Failed to activate listening.");}
    return sock;

}

int startData(int* name){   // Name will have the generated port number saved to it.
    // Create the socket
    int sock = socket(PF_INET,SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in inSockAddr;
    // Again not sure why this is necessary.
    memset(&inSockAddr, 0, sizeof(inSockAddr));
    socklen_t sizevar = (socklen_t)((sizeof(inSockAddr)));
    inSockAddr.sin_family = AF_INET;
    inSockAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any address.
    if(sock < 0){return errorHandler("Failed to create socket");}
    // Bind it to the port
    if(bind(sock,(struct sockaddr*)&inSockAddr, sizeof(inSockAddr)) < 0){
        return errorHandler("Failed to bind the socket.");
    }
    if(getsockname(sock,(struct sockaddr*)&inSockAddr,&sizevar) < 0){return errorHandler("Failed to get a socket name.");}
    *name = inSockAddr.sin_port;
    // set to listen for connections.
    if(listen(sock,1) < 0){ return errorHandler("Failed to activate listening.");}
    return sock;
}

char* getwordsocket(int socket){ 
    int rd = 1, size = 10, count = 0;
    char* clientIn = malloc(sizeof(char)*size);
    char* clsave = clientIn;
    while(rd){ 
        printf("Reading...\n");
        if((rd = read(socket,clientIn,1)) < 0){ 
            printf("Reads before failure %i\n",count);
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

int cmdPipe(int datafd,char** arguments){
    int fd[2];
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
        //dup2(fd[0],0);
        if(close(fd[1]) < 0) fprintf(stderr,"%s\n",strerror(errno));
        //execvp(args[0],args);
        int count = 1;
        char c;
        while(count){
            if(count = read(fd[0],&c,1) < 0){return errorHandler("Failed to read stuff.");}
            if(count){
                if(write(1,&c,1) < 0){return errorHandler("Failed to write stuff.");}
            }
        }


        return errorHandler("Failed to execute crap.");
    }
}



int commandHandler(char* command){
    // Stopped right here.
}
int dataHandler(int fd, int psocket){
    // Fd is the data connection.
    // psocket is the primary socket for the server.
    printf("Waiting for command in data Handler.\n");
    char* command = getwordsocket(psocket);
    if(command == NULL){return errorHandler("Failed to get a word from psocket, dataHandler.");}
    printf("Recieved Command in data Handler %s\n",command);
    if(strncmp(command,"L",2) == 0){
        // ls -l :o requires data connection first. 
        char* args[5] = {"ls","-l",NULL};
        if(cmdPipe(fd,args) < 0){return errorHandler("Failed to pipe command.");}
        return 0;
    }else if(strncmp(command,"G",2) == 0){
        // Get a file and put it in your local directoreeeeeeee, requires data connection

    }else if(strncmp(command,"P",2) == 0){
        // PUT A FILE IN THE SERVER AT ITS CURRENT WORKING DIRECTORY o_o, GIB DATA

    }
}
int dataAccept(int socket){
    struct sockaddr_in client;
    int length = sizeof(struct sockaddr_in);
    int connectionfd;
    // wait for connection, fork each time there is a connection and deal with it.
    while(1){
        if((connectionfd = accept(socket,(struct sockaddr*)&client,&length)) < 0){
            return errorHandler("Failed to accept a connection.");
        }
        if(getHost(client) < 0){errorHandler("Failed to get hostname");}
        printf("Data Connection has successfully been made!\n");
        dataHandler(connectionfd,socket);
        close(connectionfd);
    }
    return 0;
}

int connectionHandler(int socket){
    int res = fork();
    if(res < 0){
        return errorHandler("Failed to fork process.");
    }else if(!res){
        // child.
        char* command = getwordsocket(socket);
        while(command != NULL){
            if(strncmp(command,"D",2) == 0){
                // Oh dear, data connection.
                int name;
                char sName[20];// Assuming port wont be over 20 digits ¯\_(ツ)_/¯
                int datasocket = startData(&name);
                if(datasocket < 0){
                    // Bad news for the client :(
                    errorHandler("Failed to create data socket!");
                    message("E\n",socket);
                    exit(1);
                }
                sprintf(sName, "A%d\n",name);
                if(message(sName,socket) < 0){
                    // No beuno. Probably cant message that client ¯\_(ツ)_/¯
                    errorHandler("'Guess I'll die?' - Server Child 2019");
                    exit(1);
                }
                if(dataAccept(datasocket) < 0){return errorHandler("Failed to accept connection, connectionHandler");}
            }else if(strncmp(command,"C",2) == 0){
                // Changin mah directory.

            }else if(strncmp(command,"Q",2) == 0){
                // THE CLIENT WANTS YOU DEAD, feels bad man.
            
            }
            printf("Server is awaiting a command!\n");
            command = getwordsocket(socket);
            printf("Server has recieved command %s\n",command);
        }
        
        //if(message(buffer,socket) < 0){exit(1);}
        exit(0);
    }else{
        int res1;
        while((res1 = waitpid(-1,NULL,WNOHANG)) > 0);
        if(res1 < 0){
            return errorHandler("Failed to wait!");
        }
        return 0;
    }
}

int getHost(struct sockaddr_in client){
    struct hostent* hostEntry;
    hostEntry = gethostbyaddr(&(client.sin_addr),sizeof(struct in_addr),AF_INET);
    if(h_errno){
        return -1;
    }
    if(hostEntry->h_name == NULL || hostEntry->h_name[0] == '\0'){
        return -1;
    }
    printf("%s\n",hostEntry->h_name);
    return 0;
}

int message(char* msg, int socket){
    int len = strlen(msg);
    //if(send(socket,msg,(len),0) != len){return errorHandler("Didn't send expected number of bytes.");}
    int res = write(socket,msg,len+1);
    if(res != len+1){return errorHandler("Didn't write the entire message?!??!?!?!?!??!??!!!");}
    return 0;

}

