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
int connectionHandler(int socket){
    int res = fork();
    if(res < 0){
        return errorHandler("Failed to fork process.");
    }else if(!res){
        // child.
        int rd = 1, size = 10, count = 0;
        char* clientIn = malloc(sizeof(char)*size);
        char* clsave = clientIn;
        while(rd){
            if((rd = read(socket,clientIn++,1)) < 0){ return errorHandler("Read error in connectionHandler");}
            count++;
            if(count >= size && rd){
                size *= 2;
                clsave = realloc(clsave,size*sizeof(char));
            }
            if(*clientIn == '\n' || *clientIn == EOF){
            
            }
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
    if(send(socket,msg,(len),0) != len){return errorHandler("Didn't send expected number of bytes.");}
    return 0;

}
