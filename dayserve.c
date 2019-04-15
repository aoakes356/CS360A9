#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define CUST_PORT 49999

int startServer();
int message(char* msg, int socket);
int errorHandler(char* message);

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
    struck sockaddr_in inSockAddr;
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
    if(listen(sock,100) < 0){ return errorHandler("Failed to activate listening.");}
    return sock;

}

int message(char* msg, int socket){
    int len = strlen(msg);
    if(send(socket,msg,(len),0) != len){return errorHandler("Didn't send expected number of bytes.");}
    return 0;

}

