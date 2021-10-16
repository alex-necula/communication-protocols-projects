#ifndef _HELPERS_
#define _HELPERS_

#define BUFLEN 4096
#define LINELEN 1000

#include <string>

// shows the current error
void error(const char *msg);

// adds a line to a string message
void compute_message(char *message, const char *line);

// opens a connection with server host_ip on port portno, returns a socket
int open_connection(const char *host_ip, int portno, int ip_type, int socket_type, int flag);

// closes a server connection on socket sockfd
void close_connection(int sockfd);

// send a message to a server
void send_to_server(int sockfd, char *message);

// receives and returns the message from a server
char *receive_from_server(int sockfd);

// extracts and returns a JSON from a server response
char *basic_extract_json_response(char *str);

// reads integer from keyboard and checks it is actually an integer
int read_int(const char *identifier);

// reads string from keyboard
std::string read_string(const char *identifier);

// sends message to server, prints and returns response
std::string send_and_receive(char *message);

// Pretty prints Bad Request error
void print_error(std::string &response);

#endif