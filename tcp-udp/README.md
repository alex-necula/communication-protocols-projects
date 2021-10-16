# Client-Server Messaging app - TCP/UDP

## Necula Alexandru - 322CD

### Included files

- source .cpp files and headers
- Makefile containing necessary targets

### App design

- for sending messages over TCP, there is a defined struct named `TCPMessage`
- there are several types of messages, accordingly:
  - **REGISTER_CLIENT**:
    - it is sent by the client right after connecting to the server
    - field `client_id` must be filled
  - **CLOSE**:
    - it is sent by the server if the client with same ID is already connected
      or when receiving `exit` from keyboard
    - no other fields have to be filled
  - **SUBSCRIBE**:
    - it is sent by the client after receiving valid input from keyboard
    - fields `topic` and `sf` must be filled
  - **UNSUBSCRIBE**:
    - same as **SUBSCRIBE**, only `topic` must be filled
  - **MESSAGE**:
    - it is sent by the server after receiving a message from another UDP client
    - field `src_addr` and `port` will be filled with UDP client's data
    - fields `topic`, `data_type` and `content` will be filled after parsing the
      message from the UDP client
    - `content` is a union containing a struct for each `data_type`
- for storing useful data in the server efficiently, I used the STL library
- there is a struct named `TCPClient` for client's representation inside the
  server, containing:
  - active status
  - corresponding socket file descriptor
  - `unordered_set` of subscribed topics (prevents duplicates if client
    subscribes twice for the same topic)
  - `vector` for pending messages while client is disconnected and SF is 1
- for accessing data efficiently in O(1), I used STL's `unordered_map`
  - sock_map: *key* = sockfd, *val* = client's ID
  - client_map: *key* = ID, *val* = pointer to TCPClient struct
  - topic_map: *key* = name, *val* = set of subscribed clients
  - pending_map: *key* = pointer to TCPMessage struct, *val* = list of clients
    waiting for this message after reconnecting
- Pending messages are allocated on heap and deleted after all clients are
  reconnected
- Sending and receiving a message is handled by functions defined in `helpers.cpp`
