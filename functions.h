#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <netinet/tcp.h>
#include "include/common.h"
#define MAX_CONNECTIONS 100
#define BUFFER_SIZE 1551

void convert_message(struct udp_message *msg, struct tcp_data *tcp_data );

void invalid_format();

int receive_data(int sockfd, char *buf, int *len);

int send_data(int sockfd, char *buf, int len);

int connect_client(struct client *clients, int nr_clients, struct sockaddr_in cliaddr,
                   int connfd, struct pollfd *pfds, int *nfds, char *id_client);

void send_data_to_matching_clients(struct client *clients, int nr_clients, struct tcp_data *msg);

void subscribe(struct client *clients, int cl_idx, struct tcp_message *msg_received);

void unsubscribe(struct client *clients, int cl_idx, struct tcp_message *msg_received);