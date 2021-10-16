#pragma once

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#include <list>
#include <string>
#include <unordered_set>
#include <vector>

/*
 * Error verification macro
 * Example:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)  \
    do {                                  \
        if (assertion) {                  \
            fprintf(stderr, "(%s, %d): ", \
                    __FILE__, __LINE__);  \
            perror(call_description);     \
            exit(EXIT_FAILURE);           \
        }                                 \
    } while (0)

#define BUFLEN 100  // for keyboard input
#define MAX_CLIENTS 100 // for listen
#define UDP_MSG_SIZE 1551  // according to specifications
#define TCP_MSG_SIZE sizeof(TCPMessage)

#define TYPE_OFFSET 50
#define CONTENT_OFFSET 51

// Message types
#define SUBSCRIBE 0
#define UNSUBSCRIBE 1
#define REGISTER_CLIENT 2
#define CLOSE 3
#define MESSAGE 4

// Data types
#define INT 0
#define SHORT_REAL 1
#define FLOAT 2
#define STRING 3

typedef struct TCPMessage {
    char msg_type;
    char client_id[11];
    in_addr_t src_addr;
    in_port_t port;
    char topic[51];
    bool sf;
    char data_type;
    union {
        struct {
            char byte_sign;
            uint32_t value;
        } Integer;
        struct {
            uint16_t value;
        } ShortReal;
        struct {
            char byte_sign;
            uint32_t value;
            uint8_t comma_pos;
        } Float;
        struct {
            char text[1501];
        } String;
    } content;
} __attribute__((packed)) TCPMessage;

typedef struct TCPClient {
    bool active;
    int sockfd;
    std::unordered_set<std::string> sf_topics;  // store and forward subscriptions
    std::vector<TCPMessage *> pending_msgs;     // list of pending messages (if sf = 1)
} TCPClient;

int receive_tcp_message(int sockfd, TCPMessage *msg);
void send_tcp_message(int sockfd, TCPMessage *msg);
