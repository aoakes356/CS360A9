#ifndef C_MFTP_H
#define C_MFTP_H

#define CUST_PORT 49999 


// String / File IO stuff

//Get the path from the command.
char* getPath(char* command);

// Open/create a new file, returns the file descriptor.
int createFile(char* name);

// copy from one file descriptor to another.
int copyFile(int source, int destination);

// Takes file name, opens, reads, writes to datafd.
int filePipe(int datafd, char* path);

// Pipes stding into data connection after running command in arguments.
int cmdPipe(int datafd,char** arguments);

// Sends a message through a connected socket.
int message(char* msg, int socket);

// End the connection.
int closeConnection(int socket);

// Handles the errors for you.
int errorHandler(char* message);

// Get the directory from the C command.
char* getCommandDir(char* command);

// Writes all data from file descriptor into a buffer.
char* getData(int socket,int* length);

char* getFileName(char* path);

// Aquire a command from a file descriptor.
char* getwordsocket(int socket);



#endif
