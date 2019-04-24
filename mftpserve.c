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
    char c;
    while(rd){ 
        printf("Reading...\n");
        if((rd = read(socket,&c,1)) < 0){ 
            printf("Reads before failure %i\n",count);
            errorHandler("Read error in connectionHandler");
            return NULL;
        }
        clientIn[count++] = c; 
        if(count >= size && rd){
            size *= 2;
            clientIn = realloc(clientIn,size*sizeof(char));
        }
        if(clientIn[count-1] == '\n' || clientIn[count-1] == EOF || clientIn[count-1] == '\0'){
            clientIn[count-1] = '\0';
            break;
        }/*else if(clientIn[count-1] == 'D' || clientIn[count-1] == 'L' || clientIn[count-1] == 'Q'){
            clientIn[count] = '\0';
            break;
        }*/
        printf("read in: %c\n",c);
    }
    if(count <= 1){
        free(clientIn);
        return NULL;
    }
    printf("Returning this in getwordsocket%s, count: %d\n",clientIn,count);
    printf("First character of the return %i\n",clientIn[0]);
    return clientIn;

}
char* getData(int socket,int* length){
    int rd = 1, size = 10, count = 0;
    char* clientIn = malloc(sizeof(char)*size);
    char c;
    while(rd){
        if((rd = read(socket,&c,1)) < 0){
            errorHandler("Read error in connectionHandler");
            return NULL;
        }
        clientIn[count++] = c;
        if(count >= size && rd){
            size *= 2;
            clientIn = realloc(clientIn,size*sizeof(char));
        }
        if(clientIn[count-1] == EOF){
            clientIn[count-1] = '\0';
            break;
        }
    }
    if(count <= 1){
       clientIn[0] = '\0'; 
       count = 1;
    }
    //printf("Returning this in getwordsocket%s, count: %d\n",clientIn,count);
    //printf("First character of the return %i\n",clientIn[0]);
    *length = count;
    return clientIn;
}
int filePipe(int datafd, char* path){
    int fd = open(path,O_RDONLY);
    if(fd < 0){return errorHandler("Unable to open file located at path");}
    int len;
    char* data = getData(fd,&len);
    if(write(datafd,data,len) != len){return errorHandler("Failed to write entire file");}
    return 0;
}

int cmdPipe(int datafd,char** arguments){
    
    int res = fork();
    if(res < 0){return errorHandler("Failed to fork.");}
    if(!res){
        // Child.
        printf("About to send data from cmd pipe.\n");
        dup2(datafd,1);
        execvp(arguments[0],arguments);
        return errorHandler("Failed to execute command.");
    }else{
        wait(NULL);
        return 0;        
    }
    //write(datafd,"get rekt\n",10);
}

char* getPath(char* command){
    char* path = malloc(sizeof(char)*strlen(command));
    strcpy(path,&(command[1])); 
    printf("NEW PATH woo %s\n",path);
    return path;
}

int createFile(char* name){
    int fd;
    if((fd = open(name,O_WRONLY | O_CREAT | O_APPEND,0666)) < 0){return errorHandler("Failed to create the new file in current directory.");}
    printf("New file has the file descriptor %i\n",fd);
    return fd;
}

int copyFile(int source, int destination){
    char c; // Wee little buffer.
    int res;
    printf("Writing to fd: %i\n",destination);
    int len;
    char* data = getData(source,&len);
    if(data == NULL){return errorHandler("Failed to read the source file.");}
    if(write(destination,data,len) != len){return errorHandler("Error while writing destination file.");}
    return 0;
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
        if(message("A\n",psocket) < 0){return errorHandler("Failed to send accept response to the client, oh dear.");}
        char* args[5] = {"ls","-l",NULL};
        if(cmdPipe(fd,args) < 0){
           // if(message("E\n",psocket) < 0){return errorHandler("Failed to send error response to the client, wat.");}
            return errorHandler("Failed to pipe command.");
        }else{
            //if(message("A\n",psocket) < 0){return errorHandler("Failed to send accept response to the client, oh dear.");}
        }
        return 0;
    }else if(strncmp(command,"G",1) == 0){
        // Get a file and put it in your local directoreeeeeeee, requires data connection
        printf("They trynna steal our files.\n");
        char* path = getPath(command);
        if(path == NULL){return errorHandler("Failed to get a path from the command.");}
        if(message("A\n",psocket) < 0){return errorHandler("Failed to send accept response.");}
        if(filePipe(fd,path) < 0){
            if(message("ECan't get that file\n",psocket) < 0){return errorHandler("Failed to send error response to the client, wat.");} 
            return errorHandler("Failed to send file to client.");
        }
        free(path);
    }else if(command[0] == 'P'){
        // PUT A FILE IN THE SERVER AT ITS CURRENT WORKING DIRECTORY o_o, GIB DATA
        char* name = &(command[1]);
        int len;
        int new = createFile(name);
        if(new < 0){
            if(message("ECan't put that file here.\n",psocket) < 0){return errorHandler("Failed to send error response to the client, wat.");} 
        }
        if(message("A\n",psocket) < 0){return errorHandler("Failed to send accept response.");}
        if(copyFile(fd,new) < 0){return errorHandler("Failed to copy file in server.");}
    }else{
        return errorHandler("Command not recognized");
    }
}
int dataAccept(int datasocket, int psocket){
    struct sockaddr_in client;
    int length = sizeof(struct sockaddr_in);
    int connectionfd;
    // wait for connection, fork each time there is a connection and deal with it.
    if((connectionfd = accept(datasocket,(struct sockaddr*)&client,&length)) < 0){
        return errorHandler("Failed to accept a connection.");
    }
    if(getHost(client) < 0){errorHandler("Failed to get hostname");}
    printf("Data Connection has successfully been made!\n");
    dataHandler(connectionfd,psocket);
    close(connectionfd);
    return 0;
}

char* getCommandDir(char* command){
    printf("Parsing command %s\n",command);
    int len = strlen(command);
    char* dirBuffer = malloc(sizeof(char)*len);
    if(!sscanf(command,"C%s",dirBuffer)){
        dirBuffer[0] = '\0';
    }
    return dirBuffer;
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
                sprintf(sName, "A%d\n",name);
                printf("Sending server: %s",sName);
                if(message(sName,socket) < 0){
                    // No beuno. Probably cant message that client ¯\_(ツ)_/¯
                    errorHandler("'Guess I'll die?' - Server Child 2019");
                    exit(1);
                }
                if(datasocket < 0){
                    // Bad news for the client :(
                    errorHandler("Failed to create data socket!");
                    message("E\n",socket);
                    exit(1);
                }
                printf("Successfully sent, awaiting a connection\n");
                if(dataAccept(datasocket,socket) < 0){return errorHandler("Failed to accept connection, connectionHandler");}
                printf("Connection has been made!\n");
            }else if(command[0] == 'C'){
                // Changin mah directory.
                printf("Server recieved directory change request\n");
                char* dir = getCommandDir(command);
                if(dir != NULL){
                    message("A\n",socket);
                }
                printf("Changing directory to %s\n",dir);
                if(chdir(dir) < 0){
                    int size = strlen(dir)+100;
                    char* response = malloc(sizeof(char)*size);
                    sprintf(response,"E failed to change directories %s\n",dir);
                    message(response,socket);
                    free(response);
                    free(dir);
                    errorHandler("Unable to change directories");
                }/*else{
                    free(dir);
                    message("A\n",socket);
                }*/


            }else if(strncmp(command,"Q",2) == 0){
                // THE CLIENT WANTS YOU DEAD, feels bad man.
                if(message("A\n",socket) < 0){
                    // No beuno. Probably cant message that client ¯\_(ツ)_/¯
                    errorHandler("'Guess I'll die?' - Server Child 2019");
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

int message(char* msg, int socket){
    int len = strlen(msg);
    //if(send(socket,msg,(len),0) != len){return errorHandler("Didn't send expected number of bytes.");}
    int res = write(socket,msg,len);
    if(res != len){return errorHandler("Didn't write the entire message?!??!?!?!?!??!??!!!");}
    return 0;
}

