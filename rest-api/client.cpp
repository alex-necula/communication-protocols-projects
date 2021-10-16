#include <arpa/inet.h>
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <unistd.h>     /* read, write, close */

#include <iostream>
#include <limits>
#include <string>

#include "helpers.hpp"
#include "nlohmann/json.hpp"
#include "requests.hpp"

using json = nlohmann::json;
using string = std::string;

int main(int argc, char *argv[]) {
    const char *host = "34.118.48.238";
    const char *content_type = "application/json";

    string cookie;
    string token;
    char *message = NULL;

    while (1) {
        // Parse input
        string request;
        std::cin >> request;

        if (request == "register") {
            // Get data from keyboard
            string username = read_string("username");
            string password = read_string("password");

            // Compose JSON
            json j;
            j["username"] = username;
            j["password"] = password;

            // Send POST
            message = compute_post_request(host, "/api/v1/tema/auth/register", content_type, j,
                                           NULL, NULL);
            string response = send_and_receive(message);

            // Print response
            std::size_t good_reply = response.find("HTTP/1.1 201 Created");
            if (good_reply != string::npos) {
                std::cout << "Registration successful!" << std::endl;
            } else {
                print_error(response);
            }

        } else if (request == "login") {
            string username = read_string("username");
            string password = read_string("password");

            // Compose JSON
            json j;
            j["username"] = username;
            j["password"] = password;

            // Send POST
            message =
                compute_post_request(host, "/api/v1/tema/auth/login", content_type, j, NULL, NULL);
            string response = send_and_receive(message);

            // Parse cookie and print reply
            std::size_t cookie_pos = response.find("connect.sid=");
            if (cookie_pos != string::npos) {
                std::size_t end_pos = response.find(";", cookie_pos);
                cookie = response.substr(cookie_pos, end_pos - cookie_pos);

                std::cout << "Login successful!" << std::endl;
            } else {
                print_error(response);
            }

        } else if (request == "enter_library") {
            // Send GET
            message =
                compute_get_request(host, "/api/v1/tema/library/access", cookie.c_str(), NULL);
            string response = send_and_receive(message);

            // Parse JWT token and print reply
            std::size_t token_pos = response.find("{\"token\"");
            if (token_pos != string::npos) {
                json j_token = json::parse(response.substr(token_pos));
                token = j_token["token"];

                std::cout << "Authorization successful!" << std::endl;
            } else {
                print_error(response);
            }

        } else if (request == "get_books") {
            // Send GET
            message = compute_get_request(host, "/api/v1/tema/library/books", cookie.c_str(),
                                          token.c_str());
            string response = send_and_receive(message);

            // Parse response
            std::size_t books_pos = response.find("[");
            if (books_pos != string::npos) {
                std::cout << response.substr(books_pos) << std::endl;
            } else {
                print_error(response);
            }

        } else if (request == "get_book") {
            // Get data from keyboard
            int id = read_int("id");
            string url = "/api/v1/tema/library/books/" + std::to_string(id);

            // Send GET
            message = compute_get_request(host, url.c_str(), cookie.c_str(), token.c_str());
            string response = send_and_receive(message);

            // Parse response
            std::size_t book_pos = response.find("[");
            if (book_pos != string::npos) {
                std::cout << response.substr(book_pos) << std::endl;
            } else {
                print_error(response);
            }

        } else if (request == "add_book") {
            // Get data from keyboard
            string title = read_string("title");
            string author = read_string("author");
            string genre = read_string("genre");
            string publisher = read_string("publisher");
            int page_count = read_int("page_count");

            // Componse JSON
            json j = {{"title", title},
                      {"author", author},
                      {"genre", genre},
                      {"publisher", publisher},
                      {"page_count", page_count}};

            // Send POST
            message = compute_post_request(host, "/api/v1/tema/library/books", content_type, j,
                                           cookie.c_str(), token.c_str());
            string response = send_and_receive(message);

            // Parse response
            if (response.find("HTTP/1.1 200 OK") != string::npos) {
                std::cout << "Successfully added book" << std::endl;
            } else {
                print_error(response);
            }

        } else if (request == "delete_book") {
            // Get data from keyboard
            int id = read_int("id");
            string url = "/api/v1/tema/library/books/" + std::to_string(id);

            // Send DELETE
            message = compute_delete_request(host, url.c_str(), cookie.c_str(), token.c_str());
            string response = send_and_receive(message);

            // Parse response
            if (response.find("HTTP/1.1 200 OK") != string::npos) {
                std::cout << "Successfully deleted book" << std::endl;
            } else {
                print_error(response);
            }
        } else if (request == "logout") {
            // Send GET
            message = compute_get_request(host, "/api/v1/tema/auth/logout", cookie.c_str(), NULL);
            string response = send_and_receive(message);

            // Parse response
            if (response.find("HTTP/1.1 200 OK") != string::npos) {
                std::cout << "Successfully logged out" << std::endl;
            } else {
                print_error(response);
            }

            // Clear data
            cookie.clear();
            token.clear();
        } else if (request == "exit") {
            break;
        } else {
            std::cout << "Invalid request" << std::endl;
        }
    }

    return 0;
}
