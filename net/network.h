#ifndef NETWORK_H
#define NETWORK_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Functions
void initNetwork();
void cleanupNetwork();
int createSocket(int af, int type, int protocol);
int bindSocket(int sockfd, struct sockaddr* address, socklen_t addressLen);
int listenSocket(int sockfd, int backlog);
int acceptConnection(int sockfd, struct sockaddr* address, socklen_t* addressLen);
int sendData(int sockfd, const char* data, size_t length);
int receiveData(int sockfd, char* buffer, size_t length);
void printError(const char* message);

// Variables
extern int serverSockfd;
extern struct sockaddr_in serverAddress;
extern int clientSockfd;
extern struct sockaddr_in clientAddress;

#endif // NETWORK_H
