# Web Client - REST API

## Necula Alexandru - 322CD

### Included files

- source .cpp files and headers
- nlohmann JSON parser
- Makefile for compiling and running

### Usage

`make run`

### Client implementation

- the client works in an infinite loop which is stopped by entering `exit` in
the console
- the implementation takes advantage of C++'s STL (std::string)
- for creating and parsing JSON messages, I used nlohmann's parser, which
provides natural and easy JSON handling
- creating a JSON is necessary for POST requests, while parsing a JSON is useful
for printing the replies from server
- because `keep-alive` parameter is set to 5, the client opens a connection
for each request
- the client is tested with `valgrind` and has no memory leaks
- some functions are taken and adapted for C++ from the HTTP lab
