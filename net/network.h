#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>
#include <stdint.h>

// Custom types for portability
typedef int SOCKET;
typedef uint32_t in_addr_t;
typedef uint16_t in_port_t;

struct sockaddr_in {
    short          sin_family;
    in_port_t      sin_port;
    struct in_addr sin_addr;
    char           sin_zero[8];
};

struct in_addr {
    in_addr_t s_addr;
};

// Constants
#define AF_INET         2
#define SOCK_STREAM     1
#define INADDR_ANY      0
#define INVALID_SOCKET  (-1)
#define SOMAXCONN       5
#define BUF_SIZE        1024

// Error codes
#define SOCKET_ERROR    (-1)

// API Functions
int net_init(void);
void net_cleanup(void);
SOCKET socket_create(int domain, int type, int protocol);
int socket_bind(SOCKET sockfd, const struct sockaddr_in *addr);
int socket_listen(SOCKET sockfd, int backlog);
SOCKET socket_accept(SOCKET sockfd, struct sockaddr_in *addr);
int socket_send(SOCKET sockfd, const char *buf, size_t len);
int socket_recv(SOCKET sockfd, char *buf, size_t len);
void socket_close(SOCKET sockfd);

// Helper functions
in_addr_t inet_addr(const char *cp);
const char *inet_ntop(int af, const void *src, char *dst, size_t size);

// Threading
typedef void* (*thread_fn)(void*);
int thread_create(thread_fn fn, void *arg);
void thread_exit(void);

#endif // NETWORK_H
