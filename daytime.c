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

#define CUST_PORT 49999
// Returns a connected socket.
int serverConnect(char* ip);
// Sends a message through a connected socket.
int message(char* msg, int socket);
// Recieve a message from the connected server.
int getMessage(int socket);
// End the connection.
int closeConnection(int socket);
// Handles the errors for you.
int errorHandler(char* message);

int main(int argc, char** argv){
    char* ip;
    int socket;
    if(argc > 1){
        ip = argv[1];
    }else{
        printf("No ip given.\n");
        return 1;
    }
    // Get a connected socket.
    socket = serverConnect(ip);
    // Send a message.
    if(getMessage(socket) < 0) { return errorHandler("Failed to recieve message");}
    // Close the connection.
    if(closeConnection(socket) < 0){return errorHandler("Failed to close connection.");}
}

int errorHandler(char* message){ // Just a convenient method to have.
    fprintf(stderr,"CSTMERR %s: %s\n", message, strerror(errno));
    printf("%s: %s\n", message, strerror(errno));
    return -1;
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
    if(stat < 0){return errorHandler("Failed to connect to the server.");}
    return sock;
}
// Don't really need this in the client but oh well.
int message(char* msg, int socket){
    int len = strlen(msg);
    if(send(socket,msg,(len),0) != len){return errorHandler("Didn't send expected number of bytes.");}
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
// Close a socket fd.
int closeConnection(int socket){
    if(close(socket) < 0){return errorHandler("Failed to close the file descriptor.");}
    return 0;

}
