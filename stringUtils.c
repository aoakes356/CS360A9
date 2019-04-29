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
#include <sys/wait.h>
#include <fcntl.h>
#include "mftp.h"
// String utilities file.
// Stuff that is used accross both files, mostly string and file io stuff.



// get a command from a socket, reads until newline.
char* getwordsocket(int socket){
    int rd = 1, size = 10, count = 0;
    char* clientIn = malloc(sizeof(char)*size);
    char c;
    while(rd){
        printf("Reading...\n");
        if((rd = read(socket,&c,1)) < 0){
            printf("Reads before failure %i\n",count);
            errorHandler("Read error in connectionHandler");
            free(clientIn);
            return NULL;
        }
        if(rd > 0){
            clientIn[count++] = c;
            if(count >= size && rd){
                size *= 2;
                clientIn = realloc(clientIn,size*sizeof(char));
            }
            if(clientIn[count-1] == '\n' || clientIn[count-1] == '\0'){
                clientIn[count-1] = '\0';
                break;
            }
            printf("read in: %c\n",c);
        }else{
            free(clientIn);
            errorHandler("Data Connection closed by host.");
            return NULL;
        }

    }
    if(count <= 1){
        free(clientIn);
        return NULL;
    }
    printf("Returning this in getwordsocket%s, count: %d\n",clientIn,count);
    return clientIn;

}

int errorHandler(char* message){ // Just a convenient method to have.
    fprintf(stderr,"CSTMERR %s: %s\n", message, strerror(errno));
    printf("%s: %s\n", message, strerror(errno));
    return -1;
}

// Get the path from a server command
char* getPath(char* command){
    char* path = malloc(sizeof(char)*strlen(command));
    // Grabs everything but the first character of the string. 
    strcpy(path,&(command[1]));
    return path;
}

int createFile(char* name){
    int fd;
    //  Creates a file with read write permissions, or appends to an existing file.
    if((fd = open(name,O_WRONLY | O_CREAT | O_APPEND,0666)) < 0){return errorHandler("Failed to create the new file in current directory.");}
    return fd;
}
// Copy the data in source to destination.
int copyFile(int source, int destination){
    int len;
    // This copies all data from source into a buffer.
    // This is bad, but I have yet to figure out how to read one character at a time correctly.
    char* data = getData(source,&len);
    if(data == NULL){return errorHandler("Failed to read the source file.");}
    // write the data from the buffer to the desination.
    if(write(destination,data,len) != len){return errorHandler("Error while writing destination file.");}
    return 0;
}
// Like copy file except it takes a pathname instead of an open file descriptor.
int filePipe(int datafd, char* path){
    int fd = open(path,O_RDONLY);
    if(fd < 0){return errorHandler("Unable to open file located at path");}
    int len;
    char* data = getData(fd,&len);
    if(write(datafd,data,len) != len){return errorHandler("Failed to write entire file");}
    return 0;
}

// Like copy file except it a command to be executed and its output is read into data fd instead of stdout.
int cmdPipe(int datafd,char** arguments){
    int res = fork();
    if(res < 0){return errorHandler("Failed to fork.");}
    if(!res){
        // Child.
        dup2(datafd,1);
        execvp(arguments[0],arguments);
        return errorHandler("Failed to execute command.");
    }else{
        wait(NULL);
        return 0;
    }
}

// Send a message to an open file descriptor.
int message(char* msg, int socket){
    int len = strlen(msg);
    //if(send(socket,msg,(len),0) != len){return errorHandler("Didn't send expected number of bytes.");}
    int res = write(socket,msg,len);
    if(res != len){return errorHandler("Didn't write the entire message.");}
    return 0;
}

// Gets the directory passed via the changdir command. Returns an empty string if the first character is not C.
char* getCommandDir(char* command){
    int len = strlen(command);
    char* dirBuffer = malloc(sizeof(char)*len);
    if(!sscanf(command,"C%s",dirBuffer)){
        dirBuffer[0] = '\0';
    }
    return dirBuffer;
}

// Reads

char* getData(int socket,int* length){
    int rd = 1, size = 10, count = 0;
    char* clientIn = malloc(sizeof(char)*size);
    char c;
    while(rd){
        if((rd = read(socket,&c,1)) < 0){
            errorHandler("Read error in connectionHandler");
            free(clientIn);
            return NULL;
        }
        clientIn[count++] = c;
        if(count >= size && rd){
            size *= 2;
            clientIn = realloc(clientIn,size*sizeof(char));
        }
    }
    printf("Finished reading all the data, yay!\n");
    if(count <= 1){
        clientIn[0] = '\0';
        count = 1;
    }
    *length = count;
    return clientIn;
}

char* getFileName(char* path){
    int len = strlen(path), i;
    for(i = len-1; i >= 0 && path[i] != '/'; i--){
        if(path[i] == '\n'){
            path[i] = '\0';
        }
    }
    return &(path[++i]);
}

