#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <list>
#include <unordered_map>
#include <unordered_set>

#include "helpers.h"

/**
 * @brief Receives UDP message from socket
 *
 * @param sockfd socket file descriptor
 * @param buffer where message will be stored
 * @param src_addr source address
 */
void receive_udp_message(int sockfd, char *buffer, struct sockaddr *src_addr) {
    unsigned int bytes_received = 0;

    socklen_t len;
    bytes_received = recvfrom(sockfd, buffer, UDP_MSG_SIZE, MSG_DONTWAIT, src_addr, &len);
    DIE(bytes_received < 0, "recvfrom");
}

/**
 * @brief Reverses byte array
 *
 * @param res result (preallocated)
 * @param src source array
 * @param n number of bytes to reverse
 */
void reverse_char_array(char *res, char *src, int n) {
    for (int i = 0; i < n; i++) {
        res[i] = src[n - i - 1];
    }
}

/**
 * @brief Create a tcp message object using payload from UDP client
 *
 * @param buffer payload from UDP client
 * @param msg result object (preallocated)
 * @param src_addr payload's source address
 * @return int -1 in case of errors, 0 otherwise
 */
int create_tcp_message(char *buffer, TCPMessage *msg, struct sockaddr_in *src_addr) {
    // Fill struct fields
    msg->msg_type = MESSAGE;
    msg->src_addr = src_addr->sin_addr.s_addr;
    msg->port = src_addr->sin_port;

    // Parse topic
    sscanf(buffer, "%s", msg->topic);

    // Parse data_type
    msg->data_type = buffer[TYPE_OFFSET];

    // Parse content
    char byte_sign;

    switch (msg->data_type) {
        case INT: {
            byte_sign = buffer[CONTENT_OFFSET];
            if (byte_sign != 1 && byte_sign != 0) {
                return -1;
            }
            msg->content.Integer.byte_sign = byte_sign;

            char temp[4];  // buffer = big endian, temp = little endian
            reverse_char_array(temp, buffer + CONTENT_OFFSET + 1, sizeof(uint32_t));

            memcpy(&(msg->content.Integer.value), temp, sizeof(uint32_t));
        } break;
        case SHORT_REAL: {
            char temp[2];  // buffer = big endian, temp = little endian
            reverse_char_array(temp, buffer + CONTENT_OFFSET, sizeof(uint16_t));

            memcpy(&(msg->content.ShortReal.value), temp, sizeof(uint16_t));
        } break;
        case FLOAT: {
            byte_sign = buffer[CONTENT_OFFSET];
            if (byte_sign != 1 && byte_sign != 0) {
                return -1;
            }
            msg->content.Float.byte_sign = byte_sign;

            char temp[4];  // buffer = big endian, temp = little endian
            reverse_char_array(temp, buffer + CONTENT_OFFSET + 1, sizeof(uint32_t));

            memcpy(&(msg->content.Float.value), temp, sizeof(uint32_t));

            msg->content.Float.comma_pos = buffer[CONTENT_OFFSET + 5];
        } break;
        case STRING: {
            sscanf(buffer + CONTENT_OFFSET, "%[^\n]s", msg->content.String.text);
        } break;
        default:
            return -1;  // Corrupt UDP message
    }

    return 0;
}

void usage(char *file) {
    fprintf(stderr, "Usage: %s server_port\n", file);
    exit(0);
}

int main(int argc, char *argv[]) {
    int tcp_fd, udp_fd, newsockfd, portno;
    struct sockaddr_in serv_addr, cli_addr;
    int fd, ret, flag;
    socklen_t clilen = sizeof(cli_addr);
    bool exit_flag = false;

    // Disable buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    fd_set read_fds;  // Set used for reading using select()
    fd_set tmp_fds;   // Temporary set
    int fdmax;

    if (argc < 2) {
        usage(argv[0]);
    }

    portno = atoi(argv[1]);
    DIE(portno == 0, "port atoi");

    // Set struct fields for server
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // Clear file descriptor sets
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    // Open TCP listening and UDP sockets
    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(tcp_fd < 0, "socket tcp");
    DIE(udp_fd < 0, "socket udp");

    // Bind server address to TCP and UDP descriptors
    ret = bind(tcp_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "bind tcp");
    ret = bind(udp_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "bind udp");

    // Call listen for TCP descriptor
    ret = listen(tcp_fd, MAX_CLIENTS);
    DIE(ret < 0, "listen");

    // Add TCP and UDP sockets to set
    FD_SET(tcp_fd, &read_fds);
    FD_SET(udp_fd, &read_fds);
    fdmax = std::max(tcp_fd, udp_fd);

    // Add keyboard input to set
    FD_SET(STDIN_FILENO, &read_fds);

    // Map for storing file descriptors and IDs
    std::unordered_map<int, std::string> sock_map;

    // Map for storing TCP clients - client ID and its corresponding struct
    std::unordered_map<std::string, TCPClient *> client_map;

    // Map for storing topics and set of subscribers
    std::unordered_map<std::string, std::unordered_set<TCPClient *>> topic_map;

    // Map for pending messages and associated subscribers
    std::unordered_map<TCPMessage *, std::list<TCPClient *>> pending_map;

    while (1) {
        tmp_fds = read_fds;  // Save reading set (select will modify it)

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        for (fd = 0; fd <= fdmax; fd++) {
            if (FD_ISSET(fd, &tmp_fds)) {
                if (fd == tcp_fd) {
                    // New TCP client attempting to connect
                    newsockfd = accept(tcp_fd, (struct sockaddr *)&cli_addr, &clilen);
                    DIE(newsockfd < 0, "accept");

                    // Diasble Nagle's algorithm
                    flag = 1;
                    ret = setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
                    DIE(ret < 0, "setsockopt");

                    // Add new socket to reading set
                    FD_SET(newsockfd, &read_fds);
                    if (newsockfd > fdmax) {
                        fdmax = newsockfd;
                    }

                } else if (fd == udp_fd) {
                    // Received message from UDP client
                    char buffer[UDP_MSG_SIZE];
                    bzero(buffer, UDP_MSG_SIZE);
                    TCPMessage msg;
                    receive_udp_message(fd, buffer, (struct sockaddr *)&cli_addr);

                    // Process UDP message
                    ret = create_tcp_message(buffer, &msg, &cli_addr);
                    if (ret == -1) {
                        continue;  // Corrupt UDP message
                    }

                    // Send message to all subscribed TCP clients
                    std::list<TCPClient *> pending_list;
                    for (auto &client : topic_map[msg.topic]) {
                        if (client->active) {
                            send_tcp_message(client->sockfd, &msg);
                        } else {
                            // Add client to pending list for this message
                            if (client->sf_topics.count(msg.topic) == 1) {
                                pending_list.push_back(client);
                            }
                        }
                    }

                    if (!pending_list.empty()) {
                        // Allocate memory on heap for pending message
                        TCPMessage *pending_msg = new TCPMessage(msg);

                        // Add message to pending map with its clients
                        pending_map[pending_msg] = pending_list;

                        // Add message to client's pending message list
                        for (auto &client : pending_list) {
                            client->pending_msgs.push_back(pending_msg);
                        }
                    }

                } else if (fd == STDIN_FILENO) {
                    // Received input from keyboard
                    char buffer[BUFLEN];
                    bzero(buffer, BUFLEN);
                    fgets(buffer, BUFLEN - 1, stdin);

                    if (strncmp(buffer, "exit", strlen("exit")) == 0) {
                        exit_flag = true;
                        break;
                    }

                } else {
                    // Received message from TCP client
                    TCPMessage msg;
                    TCPClient *client;
                    int bytes_received = receive_tcp_message(fd, &msg);

                    // Check closed connection
                    if (bytes_received == 0) {
                        // Mark client as inactive
                        std::string id = sock_map[fd];
                        client = client_map[id];
                        client->active = false;

                        // Remove socket from reading list, socket map and close socket
                        FD_CLR(fd, &read_fds);
                        sock_map.erase(fd);
                        close(fd);

                        // Print details
                        printf("Client %s disconnected.\n", id.c_str());
                        continue;
                    }

                    // Process TCP message
                    std::string id = msg.client_id;
                    if (msg.msg_type == REGISTER_CLIENT) {
                        // Check if ID was previously registered
                        if (client_map.count(id) == 1) {
                            client = client_map[id];

                            // Check if client with same ID is connected
                            if (client->active) {
                                // Send close request as TCP message
                                TCPMessage reply_msg;
                                reply_msg.msg_type = CLOSE;
                                send_tcp_message(fd, &reply_msg);

                                // Print details
                                printf("Client %s already connected.\n", id.c_str());

                                // Remove socket from reading list
                                FD_CLR(fd, &read_fds);
                                continue;
                            } else {
                                // Client was previously connected

                                // Send pending messages
                                for (auto &msg : client->pending_msgs) {
                                    send_tcp_message(fd, msg);

                                    pending_map[msg].remove(client);

                                    // Free memory if possible
                                    if (pending_map[msg].empty()) {
                                        delete msg;
                                    }
                                }

                                // Clear pending message list
                                client->pending_msgs.clear();
                            }
                        } else {
                            // Allocate memory on heap
                            client = new TCPClient();

                            // Add client to map
                            client_map[id] = client;
                        }

                        // Update struct fields
                        client->active = true;
                        client->sockfd = fd;

                        // Add to socket map
                        sock_map[fd] = id;

                        // Print details
                        ret = getpeername(fd, (struct sockaddr *)&cli_addr, &clilen);
                        DIE(ret < 0, "getpeername");

                        printf("New client %s connected from %s:%hu.\n",
                               msg.client_id, inet_ntoa(cli_addr.sin_addr),
                               cli_addr.sin_port);

                    } else if (msg.msg_type == SUBSCRIBE) {
                        client = client_map[id];
                        topic_map[msg.topic].insert(client);
                        if (msg.sf) {
                            client->sf_topics.insert(msg.topic);
                        }
                    } else if (msg.msg_type == UNSUBSCRIBE) {
                        client = client_map[id];
                        topic_map[msg.topic].erase(client);
                    } else {
                        // Invalid message
                        continue;
                    }
                }
            }
        }

        if (exit_flag) {
            break;
        }
    }

    // Close TCP and UDP sockets
    close(tcp_fd);
    close(udp_fd);

    // Send close request to TCP clients
    TCPMessage msg;
    msg.msg_type = CLOSE;
    for (auto const &pair : sock_map) {
        int fd = pair.first;
        send_tcp_message(fd, &msg);
    }

    // Free heap memory
    for (auto const &pair : client_map) {
        delete pair.second;  // TCPClient struct
    }

    for (auto const &pair : pending_map) {
        delete pair.first;  // TCPMessage struct
    }

    return 0;
}
