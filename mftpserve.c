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
#include <fcntl.h>
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
        if((connectionfd = accept(socket,(struct sockaddr*)&client,(socklen_t*)(&length))) < 0){
            return errorHandler("Failed to accept a connection.");
        }
        if(getHost(client) < 0){errorHandler("Failed to get hostname");}
        connectionHandler(connectionfd);
	if(close(connectionfd) < 0){errorHandler("Failed to close a connection");}
    }
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




int dataHandler(int fd, int psocket){
    // Fd is the data connection.
    // psocket is the primary socket for the server.
    printf("Waiting for command in data Handler.\n");
    char* command = getwordsocket(psocket);
    if(command == NULL){return errorHandler("Failed to get a word from psocket, dataHandler.");}
    printf("Recieved Command in data Handler %s\n",command);
    if(strncmp(command,"L",2) == 0){
        // ls -l :o requires data connection first. 
        printf("We boutta LS Wooooo\n");
        char* args[5] = {"ls","-l",NULL};
        if(cmdPipe(fd,args) < 0){
            free(command);
            if(message("E\n",psocket) < 0){return errorHandler("Failed to send error response to the client, wat.");}
            return errorHandler("Failed to pipe command.");
        }else{
            if(message("A\n",psocket) < 0){
                free(command);
                return errorHandler("Failed to send accept response to the client, oh dear.");
            }
        }
        free(command);
    }else if(strncmp(command,"G",1) == 0){	// Fix me.
        // Get a file and put it in your local directoreeeeeeee, requires data connection
        printf("They trynna steal our files.\n");
        char* path = getPath(command);
        if(path == NULL){
            free(command);
            if(message("ECan't get that file\n",psocket) < 0){return errorHandler("Failed to send error response to the client, wat.");} 
            return errorHandler("Failed to get a path from the command.");
        }
        if(filePipe(fd,path) < 0){
            free(command);
            if(message("ECan't get that file\n",psocket) < 0){return errorHandler("Failed to send error response to the client, wat.");} 
            return errorHandler("Failed to send file to client.");
        }
        if(message("A\n",psocket) < 0){
            free(command);
            return errorHandler("Failed to send accept response.");
        }
        free(path);
    }else if(command[0] == 'P'){
        // PUT A FILE IN THE SERVER AT ITS CURRENT WORKING DIRECTORY o_o, GIB DATA
        char* name = &(command[1]);
        int new = createFile(name);
        if(new < 0){
            if(message("ECan't put that file here.\n",psocket) < 0){return errorHandler("Failed to send error response to the client, wat.");} 
        }
        if(message("A\n",psocket) < 0){
		if(close(new) < 0){errorHandler("Failed to close new file in put handler.");}
		return errorHandler("Failed to send accept response.");
	}
        if(copyFile(fd,new) < 0){return errorHandler("Failed to copy file in server.");}
    }else{
        free(command);
        return errorHandler("Command not recognized");
    }
    return 0;
}
int dataAccept(int datasocket, int psocket){
    struct sockaddr_in client;
    int length = sizeof(struct sockaddr_in);
    int connectionfd;
    // wait for connection, fork each time there is a connection and deal with it.
    if((connectionfd = accept(datasocket,(struct sockaddr*)&client,(socklen_t*)(&length))) < 0){
        return errorHandler("Failed to accept a connection.");
    }
    if(getHost(client) < 0){errorHandler("Failed to get hostname");}
    printf("Data Connection has successfully been made!\n");
    dataHandler(connectionfd,psocket);
    close(connectionfd);
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
                printf("Recieved data connection Request!\n");
                int name;
                char sName[20];// Assuming port wont be over 20 digits ¯\_(ツ)_/¯
                int datasocket = startData(&name);
                name = ntohs(name);
                if(datasocket < 0){
                    // Bad news for the client :(
                    errorHandler("Failed to create data socket!");
                    message("E\n",socket);
                    free(command);
                    exit(1);
                }else{
                    sprintf(sName, "A%d\n",name);
                    printf("Sending server: %s",sName);
                    if(message(sName,socket) < 0){
                        // No beuno. Probably cant message that client ¯\_(ツ)_/¯
                        errorHandler("'Guess I'll die?' - Server Child 2019");
                        free(command);
                        exit(1);
                    }
                }
                printf("Successfully sent, awaiting a connection\n");
                if(dataAccept(datasocket,socket) < 0){return errorHandler("Failed to accept connection, connectionHandler");}
                printf("Connection has been made!\n");
                free(command);
            }else if(command[0] == 'C'){
                // Changin mah directory.
                printf("Server recieved directory change request\n");
                char* dir = getCommandDir(command);
                printf("Changing directory to %s\n",dir);
                if(chdir(dir) < 0){
                    int size = strlen(dir)+100;
                    char* response = malloc(sizeof(char)*size);
                    sprintf(response,"E failed to change directories %s\n",dir);
                    message(response,socket);
                    free(response);
                    free(dir);
                    errorHandler("Unable to change directories");
                }else{
                   free(dir);
                   message("A\n",socket);
                }
                free(command);
            }else if(strncmp(command,"Q",2) == 0){
                // THE CLIENT WANTS YOU DEAD, feels bad man.
                if(message("A\n",socket) < 0){
                    // No beuno. Probably cant message that client ¯\_(ツ)_/¯
                    errorHandler("'Guess I'll die?' - Server Child 2019");
                    free(command);
                    exit(1);
                }
                break;
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



