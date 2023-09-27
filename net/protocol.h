#ifndef PROTOCOL_H
#define PROTOCOL_H

// Define the protocol version
#define PROTOCOL_VERSION "1.0"

// Define the message types
enum MessageType {
    HELLO,
    GOODBYE,
    DATA
};

// Define the messages
struct HelloMessage {
    char name[64];
};

struct GoodbyeMessage {
    char reason[64];
};

struct DataMessage {
    char data[1024];
};

// Define the function prototypes
void sendHello(int sockfd, const char* name);
void sendGoodbye(int sockfd, const char* reason);
void sendData(int sockfd, const char* data);

// Define the callback function prototype
void (*onMessage)(int sockfd, enum MessageType type, void* data);

#endif  // PROTOCOL_H
