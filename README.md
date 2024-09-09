# HW2_PCOM

This project illustrates the TCP and Multiplexing concepts and their applications in C. The program is divided into several components and files, each solving a specific problem. An attempt was made to implement the assignment as efficiently as possible, ensuring that only the necessary amount of data is transmitted for each message sent by the client or the server.

## Data Structures Used in the Created Protocols

1. `message_udp` structure: Used for interpreting messages received from UDP clients.
2. `message_tcp` structure: Used for constructing and interpreting messages transmitted between TCP clients and the server.
   - Available `message_tcp` types:
     - `ID_CLIENT`: A TCP client that has just connected has sent its name (id) to the server.
     - `SUBSCRIBE`: A TCP client has sent a request to the server to subscribe to a topic.
     - `UNSUBSCRIBE`: A TCP client has sent an unsubscribe request to the server.
     - `ANSWER_SV`: The server has responded to the request received from the TCP client with a message.
     - `INFO_SV`: The server has routed a message received from the UDP client to the TCP client.

To use memory efficiently, a union is used, which has the same size as a `message_tcp` type message and includes a `message_udp` message. This allows for easy transmission of messages received from UDP clients to the corresponding TCP clients when they are connected to the server and subscribed to topics. The `buffer` field in this union is also used as a buffer for reading commands received from the keyboard.

The `functions.h` file contains helper elements, such as character strings corresponding to subscribe/unsubscribe and exit commands, and helper functions for processing command-line arguments.

## Handling Errors and Special Cases

For handling special cases, such as:
- A TCP client tries to subscribe to a topic it's already subscribed to.
- A TCP client tries to unsubscribe from a topic it's not subscribed to.
- A TCP client tries to connect to the server with an already used ID.

A `message_tcp` message of type `ANSWER_SV` is sent to the TCP client in question, which will contain information that will be displayed to the user with the error that occurred in the command initially sent by the client.

## Subscriber and Server Functionality

The `subscriber.cpp` file is responsible for receiving messages intended for it from the server for the topics to which it is subscribed, and also for sending to the server intentions to subscribe/unsubscribe from/to a topic.

The `server.cpp` file is responsible for managing the transfer of messages between TCP clients and UDP clients, and for managing the subscribe/unsubscribe requests of TCP clients.

## Store and Forward

For implementing Store and Forward, the program uses the structures provided by STL, namely `unordered_map` and `list`. When a UDP client sends a message to which a TCP client with `SF=1` is subscribed, but which is currently disconnected, the message received from the UDP client is saved by the server in `save_SFS` so that later, when the TCP client reconnects, it can be sent to it.
