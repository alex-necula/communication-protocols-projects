#include <arpa/inet.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

#include "helpers.h"

void usage(char *file) {
    fprintf(stderr, "Usage: %s client_id server_address server_port\n", file);
    exit(0);
}

/**
 * @brief Prints received message from server according to specifications
 *
 * @param msg message object
 */
void print_tcp_message(TCPMessage *msg) {
    struct in_addr addr;
    addr.s_addr = msg->src_addr;
    printf("%s:%hu - %s - ", inet_ntoa(addr), msg->port, msg->topic);

    switch (msg->data_type) {
        case INT: {
            printf("INT - ");
            if (msg->content.Integer.byte_sign == 1) {
                printf("-");
            }
            printf("%u\n", msg->content.Integer.value);
        } break;
        case SHORT_REAL: {
            uint16_t n = msg->content.ShortReal.value;
            printf("SHORT_REAL - %u.%02u\n", n / 100, n % 100);
        } break;
        case FLOAT: {
            printf("FLOAT - ");
            if (msg->content.Float.byte_sign == 1) {
                printf("-");
            }
            uint8_t comma_pos = msg->content.Float.comma_pos;
            uint32_t n = msg->content.Float.value;

            printf("%f\n", n * pow(10, -comma_pos));
        } break;
        case STRING: {
            printf("STRING - %s\n", msg->content.String.text);
        } break;
        default: {
            printf("Invalid message\n");
        }
    }
}

int main(int argc, char *argv[]) {
    int sockfd, ret, portno;
    struct sockaddr_in serv_addr;
    std::string id = argv[1];
    char topic[51];
    bool sf;

    DIE(id.length() > 10, "ID too long");

    // Disable buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    fd_set read_fds;  // Set used for reading using select()
    fd_set tmp_fds;   // Temporary set

    if (argc < 4) {
        usage(argv[0]);
    }

    // Clear file descriptor sets
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    portno = atoi(argv[3]);
    DIE(portno == 0, "port atoi");

    // Set struct fields for server
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);

    ret = inet_aton(argv[2], &serv_addr.sin_addr);
    DIE(ret == 0, "inet_aton");

    // Open socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    // Connect to socket
    ret = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(ret < 0, "connect");

    // Send registration message
    TCPMessage msg;
    msg.msg_type = REGISTER_CLIENT;
    strcpy(msg.client_id, id.c_str());
    send_tcp_message(sockfd, &msg);

    // Add stdin and server socket to set
    FD_SET(sockfd, &read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    while (1) {
        tmp_fds = read_fds;
        ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
            // Received input from keyboard
            char buffer[BUFLEN];
            bzero(buffer, BUFLEN);
            fgets(buffer, BUFLEN - 1, stdin);

            if (strncmp(buffer, "exit", strlen("exit")) == 0) {
                break;
            }

            if (sscanf(buffer, "subscribe %s %d", topic, (int *)&sf) == 2) {
                // Send subscribe message
                TCPMessage msg;
                msg.msg_type = SUBSCRIBE;
                strcpy(msg.client_id, id.c_str());
                strcpy(msg.topic, topic);
                msg.sf = sf;

                send_tcp_message(sockfd, &msg);

                printf("Subscribed to topic.\n");
            } else if (sscanf(buffer, "unsubscribe %s", topic) == 1) {
                // Send unsubscribe message
                TCPMessage msg;
                msg.msg_type = UNSUBSCRIBE;
                strcpy(msg.client_id, id.c_str());
                strcpy(msg.topic, topic);

                send_tcp_message(sockfd, &msg);

                printf("Unsubscribed from topic.\n");
            }

        } else if (FD_ISSET(sockfd, &tmp_fds)) {
            // Received message from server
            TCPMessage msg;
            receive_tcp_message(sockfd, &msg);

            if (msg.msg_type == CLOSE) {
                break;
            } else if (msg.msg_type == MESSAGE) {
                print_tcp_message(&msg);
            }
        }
    }

    // Close socket
    close(sockfd);

    return 0;
}
