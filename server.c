#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <netinet/tcp.h>
#include "include/common.h"
#include "include/utils.h"
#include "functions.h"

void free_func(struct client *clients, int nr_clients) {
    for (int i = 0; i < nr_clients; i ++) {
        free(clients[i].topics);
        free(clients[i].messages);
    }
    free(clients);
}


int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <SERVER_PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sock_tcp, sock_udp;
    struct sockaddr_in servaddr_tcp, servaddr_udp, cliaddr;
    socklen_t clilen = sizeof(cliaddr);

    sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sock_tcp < 0, "socket TCP");

    memset(&servaddr_tcp, 0, sizeof(servaddr_tcp));
    servaddr_tcp.sin_family = AF_INET;
    servaddr_tcp.sin_addr.s_addr = INADDR_ANY;
    servaddr_tcp.sin_port = htons(atoi(argv[1]));

    // Make port reusable
    int set = 1;
    int rc = setsockopt(sock_tcp, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(int));
    DIE(rc < 0, "setsockopt SO_REUSEADDR");
    
    // Disable Nagle's algorithm
    rc = setsockopt(sock_tcp, IPPROTO_TCP, TCP_NODELAY, &set, sizeof(int));
    DIE(rc < 0, "setsockopt TCP_NODELAY");

    rc = bind(sock_tcp, (struct sockaddr *)&servaddr_tcp, sizeof(servaddr_tcp));
    DIE(rc < 0, "bind TCP");

    rc = listen(sock_tcp, MAX_CONNECTIONS);
    DIE(rc < 0, "listen");

    sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(sock_udp < 0, "socket UDP");

    memset(&servaddr_udp, 0, sizeof(servaddr_udp));
    servaddr_udp.sin_family = AF_INET;
    servaddr_udp.sin_addr.s_addr = INADDR_ANY;
    servaddr_udp.sin_port = htons(atoi(argv[1]));

    rc = bind(sock_udp, (struct sockaddr *)&servaddr_udp, sizeof(servaddr_udp));
    DIE(rc < 0, "bind UDP");

    struct pollfd pfds[MAX_CONNECTIONS];
    pfds[0].fd = sock_tcp;
    pfds[0].events = POLLIN;
    pfds[1].fd = sock_udp;
    pfds[1].events = POLLIN;
    pfds[2].fd = 0; // stdin
    pfds[2].events = POLLIN;
    int nfds = 3;

    struct client *clients = calloc(1, sizeof(struct client));
    if (clients == NULL)
    {
        perror("calloc error");
        return 1;
    }
    int nr_clients = 0;
    while (1)
    {
        rc = poll(pfds, nfds, -1);
        if (rc < 0)
        {
            perror("Poll error");
            break;
        }

        for (int i = 0; i < nfds; i++)
        {
            if (pfds[i].revents & POLLIN)
            {
                
                if (pfds[i].fd == sock_tcp)
                {
                    int connfd = accept(sock_tcp, (struct sockaddr *)&cliaddr, &clilen);
                    if (connfd < 0)
                    {
                        perror("Accept error");
                        break;
                    }

                    // Disable Nagle's algorithm
                    int set = 1;
                    rc = setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, &set, sizeof(int));
                    DIE(rc < 0, "setsockopt TCP_NODELAY");

                    char buffer[BUFFER_SIZE];
                    memset(buffer, 0, BUFFER_SIZE);

                    int len = 0;
                    char id_client[11];

                    rc = receive_data(connfd, &id_client, &len);

                    if (rc < 0)
                    {
                        perror("Read error!");
                        close(connfd);
                        continue;
                    }
                    strcpy(id_client + len,"\0");

                    // Needs to verify if client is already connected
                    int client_exist = connect_client(clients, nr_clients, cliaddr, connfd, pfds, &nfds, id_client);

                    if (!client_exist)
                    {
                        clients = realloc(clients, (nr_clients + 1) * sizeof(struct client));
                        if (clients == NULL)
                        {
                            perror("realloc error");
                            break;
                        }
                        struct client c;
                        strcpy(c.id, id_client);
                        c.socket = connfd;
                        c.is_connected = 1;
                        c.nr_topics = 0;
                        c.nr_msg = 0;
                        c.topics = NULL;
                        c.messages = NULL;
                        clients[nr_clients++] = c;

                        // Print client connected mesage
                        printf("New client %s connected from %s:%d.\n", id_client, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

                        // Add the socket to poll
                        pfds[nfds].fd = connfd;
                        pfds[nfds].events = POLLIN;
                        nfds++;
                    }
                }
                else if (pfds[i].fd == sock_udp)
                {
                    char buffer[BUFFER_SIZE];
                    memset(buffer, 0, BUFFER_SIZE);

                    int len = recvfrom(sock_udp, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&cliaddr, &clilen);
                    if (len < 0)
                    {
                        perror("Recvfrom error");
                        continue;
                    }
                    struct udp_message *msg = (struct udp_mesage *)buffer;
                    struct tcp_data tcp_data;
                    
                    // Convert data to correct type
                    convert_message(msg, &tcp_data);

                    strcpy(tcp_data.ip, inet_ntoa(cliaddr.sin_addr));
                    tcp_data.port = ntohs(cliaddr.sin_port);
                    strcpy(tcp_data.topic, msg->topic);

                    // Send data to clients
                    send_data_to_matching_clients(clients, nr_clients, &tcp_data);
                }
                // Read data from input
                else if (pfds[i].fd == 0)
                {
                    char buffer[BUFFER_SIZE];
                    memset(buffer, 0, BUFFER_SIZE);
                    int n = read(0, buffer, sizeof(buffer));
                    if (n < 0)
                    {
                        perror("Read error from stdin");
                        break;
                    }

                    if (strcmp(buffer, "exit\n") == 0)
                    {
                        // Close the clients and the server
                        for (int idx = 0; idx < nr_clients; idx++)
                        {
                            if (clients[idx].is_connected)
                            {
                                close(clients[idx].socket);
                            }
                        }
                        free_func(clients, nr_clients);
                        close(sock_tcp);
                        return 0;
                    }
                    else
                    {
                        fprintf(stderr, "Command is not allowed! \n");
                    }
                } 
                // subscribe/usubscribe messages
                else if (pfds[i].fd != sock_udp)
                {
                    int flag = 1;
                    rc = setsockopt(pfds[i].fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
                    if (rc < 0)
                    {
                        perror("setsockopt TCP_NODELAY");
                        break;
                    }

                    struct tcp_message msg_received;
                    char buffer[BUFFER_SIZE];
                    memset(buffer, 0, BUFFER_SIZE);
                    int len = 0;
                    rc = receive_data(pfds[i].fd, buffer, &len);

                    if (rc < 0)
                    {
                        perror("Read error from client");
                        break;
                    }

                    int cl_idx = 0;
                    for (cl_idx = 0; cl_idx < nr_clients; cl_idx++)
                    {
                        if (clients[cl_idx].socket == pfds[i].fd)
                        {
                            break;
                        }
                    }

                    if (len == 0)
                    {
                        // Client disconnected
                        printf("Client %s disconnected.\n", clients[cl_idx].id);
                        close(pfds[i].fd);
                        clients[cl_idx].is_connected = 0;
                        continue;
                    }

                    msg_received = *(struct tcp_message *)buffer;
                    if (strcmp(msg_received.action, "subscribe") == 0)
                    {
                        subscribe(clients, cl_idx, &msg_received);
                    }

                    if (strcmp(msg_received.action, "unsubscribe") == 0)
                    {
                        unsubscribe(clients, cl_idx, &msg_received);
                    }
                }
            }
        }
    }

    // free allocated memory 
    free_func(clients, nr_clients);

    close(sock_tcp);
    close(sock_udp);

    return 0;
}
