#include "helpers.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

/**
 * @brief Receives a TCP message from socket
 *
 * @param sockfd socket file descriptor
 * @param msg pointer where message will be received
 * @return int bytes received
 */
int receive_tcp_message(int sockfd, TCPMessage *msg) {
    unsigned int bytes_received = 0;
    unsigned int bytes_remaining = TCP_MSG_SIZE;
    char buffer[TCP_MSG_SIZE];
    unsigned int n;

    while (bytes_received < TCP_MSG_SIZE) {
        n = recv(sockfd, buffer + bytes_received, bytes_remaining, 0);
        DIE(n < 0, "recv");

        if (n == 0) {
            return 0;  // Connection closed
        }

        bytes_received += n;
        bytes_remaining -= n;
    }

    // Copy buffer to struct (ensured packing)
    memcpy(msg, buffer, TCP_MSG_SIZE);

    return bytes_received;
}

/**
 * @brief Sends TCP message to socket
 *
 * @param sockfd socket file descriptor
 * @param msg pointer where message is stored
 */
void send_tcp_message(int sockfd, TCPMessage *msg) {
    unsigned int bytes_sent = 0;
    unsigned int bytes_remaining = TCP_MSG_SIZE;
    char buffer[TCP_MSG_SIZE];
    unsigned int n;

    // Copy message into buffer
    memcpy(buffer, msg, TCP_MSG_SIZE);

    while (bytes_sent < TCP_MSG_SIZE) {
        n = send(sockfd, buffer + bytes_sent, bytes_remaining, 0);
        DIE(n < 0, "send");

        bytes_sent += n;
        bytes_remaining -= n;
    }
}