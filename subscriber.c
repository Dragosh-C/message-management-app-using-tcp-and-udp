#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <netinet/tcp.h>
#include "include/common.h"
#include "functions.h"
#include "include/utils.h"
#define BUFFER_S 2000


void subscribe_to_topic(struct tcp_message msg_to_sent, int client_sock, char *client_id, char *p) {
    strcpy(msg_to_sent.action, p);
    p = strtok(NULL, " ");
    if (p == NULL) {
        invalid_format();
        return;
    }
    strcpy(msg_to_sent.msg.name, p);
    p = strtok(NULL, " ");
    if (p == NULL) {
        invalid_format();
        return;
    }
    
    msg_to_sent.msg.sf = atoi(p);
    if(msg_to_sent.msg.sf != 0 && msg_to_sent.msg.sf != 1) {
        fprintf(stderr, "Invalid comand. <TF> must be 0 or 1\n");
        return;
    }
    p = strtok(NULL, " ");
    if(p != NULL) {
        invalid_format();
        return;
    }
    strcpy(msg_to_sent.client_id, client_id);
    int rc = send_data(client_sock, &msg_to_sent, sizeof(struct tcp_message));
    if (rc < 0) {
        perror("Error sending data to server");
        return;
    }
                
    printf("Subscribed to topic.\n");
}

void unsubscribe_from_topic(struct tcp_message msg_to_sent, int client_sock, char *client_id, char *p) {
    strcpy(msg_to_sent.action, p);
    p = strtok(NULL, " \n");
    if (p == NULL || strcmp(p, "\n") == 0) {
        invalid_format();
        return;
    }
    
    strcpy(msg_to_sent.msg.name, p);
    msg_to_sent.msg.sf = -1;
    p = strtok(NULL, " ");
    
    if (p != NULL) {
         invalid_format();
        return;
    }
    strcpy(msg_to_sent.client_id, client_id);
    
    int rc = send_data(client_sock, &msg_to_sent, sizeof(struct tcp_message));
    
    if (rc < 0) {
        perror("Error sending data to server");
        return;
    }
    printf("Unsubscribed from topic.\n");
}


int main(int argc, char *argv[]) {

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int client_sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_S];

    if (argc != 4) {
        fprintf(stderr, "Usage: ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n");
        return 0;
    }

    // Create socket
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    DIE(client_sock < 0, "socket");
    
    // Disable Nagle's algorithm
    int set = 1;
    setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, &set, sizeof(int));
    DIE(set < 0, "setsockopt");

    // Set server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[2]);
    server_addr.sin_port = htons(atoi(argv[3]));

    // Connect to server
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        return 0;
    }
    
    
    char *client_id = argv[1];
    if (strlen(client_id) > 10) {
        fprintf(stderr, "Client ID too long\n");
        close(client_sock);
        return 0;
    }

    // Send client ID to server
    if (send_data(client_sock, client_id, strlen(client_id)) < 0) {
        perror("Error sending data to server");
        return 0;
    }
    
    struct pollfd pfds[MAX_CONNECTIONS];
    pfds[0].fd = 0;  // stdin
    pfds[0].events = POLLIN;
    pfds[1].fd = client_sock;
    pfds[1].events = POLLIN;
    int nfds = 3;

    while (1) {
        int rc = poll(pfds, nfds, -1);
        if (rc < 0) {
            perror("poll error");
            break;
        }

        if (pfds[0].revents & POLLIN) {

            // Read from stdin
            memset(buffer, 0, BUFFER_S);
            read(0, buffer, sizeof(buffer));
            if (strcmp(buffer, "exit\n") == 0) {
                close(client_sock);
                return 0;
            }
            struct tcp_message msg_to_sent;

            char *p = strtok(buffer, " ");

            // Subscribe/unsubscribe to a topic
            if (strcmp(p, "subscribe") == 0) {

                subscribe_to_topic(msg_to_sent, client_sock, client_id, p);

            } else if (strcmp(p, "unsubscribe") == 0) {
                
                unsubscribe_from_topic(msg_to_sent, client_sock, client_id, p);

            } else {
                invalid_format();
            }
            
        }

        if (pfds[1].revents & POLLIN) {
            // Read from server
            int len = 0;
            memset(buffer, 0, BUFFER_S);

            rc = receive_data(client_sock, buffer, &len);
            if (rc < 0) {
                perror("Error receiving data from server");
                break;
            }

            if (len == 0) {
                close(client_sock);
                return 0;
            }
                struct tcp_data *msg = (struct tcp_data *) buffer;
                printf("%s:%d - %s - %s - %s\n", msg->ip, msg->port, msg->topic, msg->type, msg->buff);
        }    
    }

    close(client_sock);

    return 0;
}
