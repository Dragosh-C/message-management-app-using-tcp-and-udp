TCP and UDP server client application for message management


For data transmission between client and server, I have defined several different structures for all of message
types, they are in the common.h header. The functions used in the code are added in functions.c file.

First, I opened a TCP and UDP socket, I made a bind to the specified address and port, and I used listen
for connections. For IO multiplexing, I used poll and added the TCP and UDP sockets to the list of
file descriptors. I used poll to wait for new connections and to receive messages from clients.

After in a loop I checked if an event occurred on one of the TCP, UDP or stdin sockets and I have
treated each case separately. For TCP sockets, I accepted the connection and added the socket to the list of files
descriptors. I've disabled Nagle's algorithm so I can send messages faster. To be able to receive
data efficiently, I created two functions receive_data and send_data. send_data first sends the size
of the message and then the message, as well as receive_data, first receives the size of the message and then sends the data
and check if everything has been sent.

After receiving the tcp data in a buffer, I cast to the corresponding structure and treated each one
individual case. To check if a client is already connected, I implemented the function connect_client in which
I also displayed the messages for clients that were disconnected and had sf set to 1. If the client does not exist
I added the client to the list of clients and updated the list of sockets.

For the UDP sockets, I received the data in a buffer and checked the corresponding structure. I created a
convert_message function that converts the received message to the appropriate type and adds them to the tcp_data structure
to be sent to customers. Further, in the send_data_to_mathing_clients function, I went through the list of
clients and I checked the topic to which they are subscribed if the client was connected and the topic coincided with that of
message, send the message to the client. Otherwise, I checked if it has active sf and saved the message in the list of
messages for that client.

For the stdin socket I read from the keyboard and checked if the command is exit, if so I freed the memory
and I closed the server together with all the clients.
Then I received subscribe or unsubscribe messages, for this I implemented two functions, subscribe and
unsubscribe in which I added or deleted from the clients topic.

I used message structures over TCP. These are udp_message, tcp_message, tcp_data. Each one was used
for a message type. I used a data structure to store the clients (struct client), in which I saved
the socket, the id, the topics to which it is subscribed and the messages that must be sent when 
sf = 1 if the client is not logged in.

In the subscribe.c file, I also opened the necessary socket and made the connection. Likewise, I used poll to
wait for messages from the server and from stdin. First I sent a message with the client id. After I implemented
the functions subscribe_to_topic and unsubscribe_from_topic in which I sent the messages with the necessary topic 
to the server. Then I received the messages from the server and displayed them.
