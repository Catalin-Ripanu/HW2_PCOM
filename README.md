# HW2_PCOM

The program is divided into several components and files, each solving a specific problem. An attempt was made to implement the assignment as efficiently as possible. Thus, for all messages sent either by the client or by the server, only as many bytes as necessary will be sent to completely transmit the information and nothing more.

In the source files, found in the archive, are the implementations, with the more important parts of the subscriber and server program logic being commented.

## Data structures used in the created protocols:

- The `message_udp` structure is used for interpreting messages received from UDP clients.
- The `message_tcp` structure is used for constructing and interpreting messages transmitted between TCP clients and the server.

-> The available `message_tcp` types are as follows:
- ID_CLIENT: a TCP client that has just connected has sent its name (id) to the server.
- SUBSCRIBE: a TCP client has sent a request to the server to subscribe to a topic.
- UNSUBSCRIBE: a TCP client has sent an unsubscribe request to the server.
- ANSWER_SV: the server has responded to the request received from the TCP client with a message.
- INFO_SV: the server has routed a message received from the UDP client to the TCP client.

For efficient memory use, I used a union that has the same size as a `message_tcp` type message and which includes in the TCP message a `message_udp` message to easily transmit messages received from UDP clients to the corresponding TCP clients when they are connected to the server and subscribed to topics. The buffer field in this union is also used as a buffer for reading commands received from the keyboard.

In the `functions.h` file, helper elements are stored, such as character strings corresponding to subscribe/unsubscribe and exit commands, helper functions for processing command line arguments, etc.

-> Errors/Special cases:
For handling special cases such as "a TCP client tries to subscribe to a topic it's already subscribed to", "a TCP client tries to unsubscribe from a topic it's not subscribed to", "a TCP client tries to connect to the server with an already used ID", a `message_tcp` message of type ANSWER_SV will be sent to the TCP client in question, which will contain information that will be displayed to the user with the error that occurred in the command initially sent by the client.

Obviously, the function of the `subscriber.cpp` file is to receive messages intended for it from the server for the topics to which it is subscribed, and also to send to the server intentions to subscribe/unsubscribe from/to a topic.

The function of the `server.cpp` file is to manage the transfer of messages between TCP clients and UDP clients and to manage the subscribe/unsubscribe requests of TCP clients.

-> Store and Forward:
For implementing Store and Forward, I used the structures provided by STL, namely unordered_map and list (I could have also used a queue). When a UDP client sends a message to which a TCP client with SF=1 is subscribed, but which is currently disconnected, the message received from the UDP client is saved by the server in save_SFS so that later, when the TCP client reconnects, it can be sent to it.
