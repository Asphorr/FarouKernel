#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#define PORT 8080
#define BACKLOG 5
#define BUF_SIZE 1024

// Function declarations
void* handle_client(void *arg);
int setup_server_address(struct sockaddr_in *server);
int initialize_server_socket();
void server_cleanup(int server_fd);

// Thread argument structure
typedef struct {
    int socket;
    struct sockaddr_in address;
} client_data_t;

// Initialize server address structure
int setup_server_address(struct sockaddr_in *server) {
    memset(server, 0, sizeof(*server));
    server->sin_family = AF_INET;
    server->sin_addr.s_addr = INADDR_ANY;
    server->sin_port = htons(PORT);

    return 0;
}

// Initialize server socket and return the file descriptor
int initialize_server_socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Server: Error creating socket: %s\n", strerror(errno));
        return -1;
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        fprintf(stderr, "Server: Error setting socket options: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    struct sockaddr_in server_address;
    if (setup_server_address(&server_address) != 0) {
        close(sockfd);
        return -1;
    }

    if (bind(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        fprintf(stderr, "Server: Error binding socket: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, BACKLOG) < 0) {
        fprintf(stderr, "Server: Error listening on socket: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    return sockfd;
}

// Handle client connection
void* handle_client(void *arg) {
    client_data_t *client = (client_data_t *)arg;
    int client_fd = client->socket;
    struct sockaddr_in client_addr = client->address;
    free(arg);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    printf("Server: Got connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port));

    char buffer[BUF_SIZE];
    while (1) {
        memset(buffer, 0, BUF_SIZE);
        int bytes_read = recv(client_fd, buffer, BUF_SIZE - 1, 0);
        if (bytes_read < 0) {
            fprintf(stderr, "Server: Error reading from client %s: %s\n", client_ip, strerror(errno));
            break;
        } else if (bytes_read == 0) {
            printf("Server: Client %s disconnected\n", client_ip);
            break;
        }

        printf("Server: Received message from %s - %s\n", client_ip, buffer);
        char *message = "Hello, client!";
        if (send(client_fd, message, strlen(message), 0) < 0) {
            fprintf(stderr, "Server: Error sending message to client %s: %s\n", client_ip, strerror(errno));
            break;
        }
    }

    close(client_fd);
    return NULL;
}

// Signal handler for cleaning up server socket
void server_cleanup(int server_fd) {
    printf("Server: Cleaning up resources...\n");
    close(server_fd);
    exit(0);
}

int main() {
    // Set up signal handling for clean exit
    signal(SIGINT, (void (*)(int))server_cleanup);

    int server_fd = initialize_server_socket();
    if (server_fd < 0) {
        fprintf(stderr, "Server: Failed to initialize server socket\n");
        return 1;
    }

    printf("Server: Listening on port %d...\n", PORT);

    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_address, &client_len);
        if (client_fd < 0) {
            fprintf(stderr, "Server: Failed to accept incoming connection: %s\n", strerror(errno));
            continue;
        }

        // Allocate memory for thread argument
        client_data_t *client_data = malloc(sizeof(client_data_t));
        if (!client_data) {
            fprintf(stderr, "Server: Failed to allocate memory for client data\n");
            close(client_fd);
            continue;
        }

        client_data->socket = client_fd;
        client_data->address = client_address;

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, client_data) != 0) {
            fprintf(stderr, "Server: Failed to create thread for new client: %s\n", strerror(errno));
            close(client_fd);
            free(client_data);
            continue;
        }

        // Detach the thread to free resources upon completion
        pthread_detach(client_thread);
    }

    server_cleanup(server_fd);
    return 0;
}
