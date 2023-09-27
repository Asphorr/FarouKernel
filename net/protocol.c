#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocol.h"

// Function to send a hello message
void sendHello(int sockfd, const char* name) {
    char message[] = "hello";
    strcat(message, name);
    strcat(message, "\n");
    send(sockfd, message, strlen(message), 0);
}

// Function to send a goodbye message
void sendGoodbye(int sockfd, const char* reason) {
    char message[] = "goodbye";
    strcat(message, reason);
    strcat(message, "\n");
    send(sockfd, message, strlen(message), 0);
}

// Function to send data
void sendData(int sockfd, const char* data) {
    send(sockfd, data, strlen(data), 0);
}

// Function to receive a message
char* receiveMessage(int sockfd) {
    char buffer[1024];
    recv(sockfd, buffer, 1024, 0);
    return buffer;
}

// Function to parse a message
void parseMessage(char* message) {
    char* token = strtok(message, "\n");
    if (token == NULL) {
        printf("Invalid message format\n");
        return;
    }
    if (strcmp(token, "hello") == 0) {
        // Handle hello message
    } else if (strcmp(token, "goodbye") == 0) {
        // Handle goodbye message
    } else if (strcmp(token, "data") == 0) {
        // Handle data message
    } else {
        printf("Unknown message type\n");
    }
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);

    // Connect to server
    connect(sockfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress));

    // Send hello message
    char name[] = "John Doe";
    sendHello(sockfd, name);

    // Receive message
    char* message = receiveMessage(sockfd);
    parseMessage(message);

    // Send goodbye message
    sendGoodbye(sockfd, "Thanks for chatting!");

    // Close socket
    close(sockfd);

    return 0;
}
