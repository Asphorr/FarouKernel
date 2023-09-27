#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(client);
    char buffer[256];

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Failed to create socket");
        exit(1);
    }

    // Set address and port number for server
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    // Bind socket to address and port
    if (bind(server_fd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("Failed to bind socket");
        exit(1);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Failed to listen for incoming connections");
        exit(1);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        // Accept incoming connection
        client_fd = accept(server_fd, (struct sockaddr*)&client, &client_len);
        if (client_fd < 0) {
            perror("Failed to accept incoming connection");
            exit(1);
        }

        // Read data from client
        read(client_fd, buffer, 256);
        printf("Client message: %s\n", buffer);

        // Send response back to client
        char* message = "Hello, client!";
        send(client_fd, message, strlen(message), 0);

        // Close client socket
        close(client_fd);
    }

    return 0;
}
