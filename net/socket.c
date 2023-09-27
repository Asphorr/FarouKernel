#include "socket.h"

int create_socket(int domain, int type, int protocol) {
    int sockfd;
    sockfd = socket(domain, type, protocol);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void bind_socket(int sockfd, struct sockaddr_in *address, socklen_t address_len) {
    if (bind(sockfd, (struct sockaddr *)address, address_len) < 0) {
        perror("binding failed");
        exit(EXIT_FAILURE);
    }
}

void listen_socket(int sockfd, int backlog) {
    if (listen(sockfd, backlog) < 0) {
        perror("listening failed");
        exit(EXIT_FAILURE);
    }
}

client_t accept_socket(int sockfd) {
    client_t client;
    socklen_t client_len = sizeof(client.addr);
    client.fd = accept(sockfd, (struct sockaddr *)&client.addr, &client_len);
    if (client.fd < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
    return client;
}

ssize_t read_socket(int sockfd, void *buffer, size_t length) {
    ssize_t ret = read(sockfd, buffer, length);
    if (ret < 0) {
        perror("reading failed");
        exit(EXIT_FAILURE);
    }
    return ret;
}

ssize_t write_socket(int sockfd, const void *buffer, size_t length) {
    ssize_t ret = write(sockfd, buffer, length);
    if (ret < 0) {
        perror("writing failed");
        exit(EXIT_FAILURE);
    }
    return ret;
}

int close_socket(int sockfd) {
    if (close(sockfd) < 0) {
        perror("closing failed");
        exit(EXIT_FAILURE);
    }
    return 0;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[256];

    // Create a socket
    server_fd = create_socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to a specific address and port
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bind_socket(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // Listen for incoming connections
    listen_socket(server_fd, 3);

    printf("Server listening on port 8080...\n");

    while (1) {
        // Accept an incoming connection
        client_fd = accept_socket(server_fd);
        if (client_fd < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        // Read data from the client
        read_socket(client_fd, buffer, 256);
        printf("Client message: %s\n", buffer);

        // Write a response back to the client
        char* message = "Hello, client!";
        write_socket(client_fd, message, strlen(message));

        // Close the client socket
        close_socket(client_fd);
    }

    return 0;
}
