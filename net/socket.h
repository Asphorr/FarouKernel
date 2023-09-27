#ifndef _SOCKET_H
#define _SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENT_LENGTH 256

// Structure to store client information
typedef struct {
    int fd;
    struct sockaddr_in addr;
} client_t;

// Function prototypes
int create_socket(int domain, int type, int protocol);
void bind_socket(int sockfd, struct sockaddr_in *address, socklen_t address_len);
void listen_socket(int sockfd, int backlog);
client_t accept_socket(int sockfd);
ssize_t read_socket(int sockfd, void *buffer, size_t length);
ssize_t write_socket(int sockfd, const void *buffer, size_t length);
int close_socket(int sockfd);

#endif // _SOCKET_H
