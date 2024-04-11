#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BACKLOG 3
#define BUF_SIZE 256

void setup_server_address(struct sockaddr_in *server) {
    memset(server, 0, sizeof(*server));
    server->sin_family = AF_INET;
    server->sin_addr.s_addr = INADDR_ANY;
    server->sin_port = htons(PORT);
}

int initialize_server_socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Server: Error creating socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Server: Error setting socket options");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    setup_server_address(&server_address);

    if (bind(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Server: Error binding socket");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, BACKLOG) < 0) {
        perror("Server: Error listening on socket");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

void handle_client(int client_fd) {
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);
    int bytes_read = read(client_fd, buffer, BUF_SIZE - 1);
    if (bytes_read < 0) {
        perror("Server: Error reading from client");
        return;
    }

    printf("Server: Received message - %s\n", buffer);
    char *message = "Hello, client!";
    if (send(client_fd, message, strlen(message), 0) < 0) {
        perror("Server: Error sending message to client");
    }
}

int main() {
    int server_fd = initialize_server_socket();
    printf("Server: Listening on port %d...\n", PORT);

    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_address, &client_len);
        if (client_fd < 0) {
            perror("Server: Failed to accept incoming connection");
            continue;
        }

        handle_client(client_fd);
        close(client_fd);
    }

    // Close server socket
    close(server_fd);
    return 0;
}
