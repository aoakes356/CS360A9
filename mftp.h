#ifndef C_MFTP_H
#define C_MFTP_H

#define CUST_PORT 49999 

// Sends a message through a connected socket.
int message(char* msg, int socket);
// Recieve a message from the connected server.
int getMessage(int socket);
// End the connection.
int closeConnection(int socket);
// Handles the errors for you.
int errorHandler(char* message);
// Change the directory via C with error handling.
int changeDirectory(char* path);
// Show the contents of the given directory.
int printD();


#endif
