#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define CUST_PORT 49999

int serverConnect(char* ip);
int errorHandler(char* message);

int main(int argc, char** argv){
    char* ip;
    if(argc > 1){
        ip = argv[1];
    }else{
        printf("No ip given.\n");
        return 1;
    }
    serverConnect(ip);
}

int errorHandler(char* message){ // Just a convenient method to have.
    fprintf(stderr,"CSTMERR %s: %s\n", message, strerror(errno));
    printf("%s: %s\n", message, strerror(errno));
    return -1;
}

int serverConnect(char* ip){

}
