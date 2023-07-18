#include "functions.h"


void convert_message(struct udp_message *msg, struct tcp_data *tcp_data ) {
    // Convert the message to correct format
    if (msg->type == 0) {
        strcpy(tcp_data->type, "INT");    
        int nr = ntohl(*(uint32_t *)(msg->buff + 1));
        if(msg->buff[0] == 1)
        nr *= -1;
        sprintf(tcp_data->buff, "%d", nr);
    }

    if (msg->type == 1) {
        strcpy(tcp_data->type, "SHORT_REAL");
        double nr = ntohs(*(uint16_t *)(msg->buff));
        nr /= 100;
        sprintf(tcp_data->buff, "%.2f", nr);
    }

    if (msg->type == 2) {
        strcpy(tcp_data->type, "FLOAT");
        double nr = ntohl(*(uint32_t *)(msg->buff + 1));
        int p = 1;
        for(int i = 1; i <= msg->buff[5]; i++) {
            p *= 10;
        }
        nr /= p;

        if(msg->buff[0] == 1) {
            nr *= -1;
        }
        sprintf(tcp_data->buff, "%lf", nr);
    }

    if (msg->type == 3) {
        strcpy(tcp_data->type, "STRING");
        strcpy(tcp_data->buff, msg->buff);
    }
}

void invalid_format() {
    fprintf(stderr, "Invalid command! Enter: Subscribe <Topic> <TF>\n");
}

int receive_data(int sock, char *buffer, int *len){
	
	int rc = recv(sock, len, sizeof(int), 0);
	
    if (rc <= 0) {
        return rc;
    }

	int nr_bytes_to_sent = *len;
	int nr_bytes_received = 0;

    while (nr_bytes_received < *len) {
        rc = recv(sock, buffer + nr_bytes_received, nr_bytes_to_sent, 0);
        if(rc < 0) {
            return rc;
        }
        nr_bytes_received += rc;
        nr_bytes_to_sent -= rc;
    }
    return 1;
}

int send_data(int sock, char *buffer, int len){
    int nr_bytes_sent = 0;
    int nr_bytes_to_sent = len;

    int rc = send(sock, &len, sizeof(int), 0);
    if (rc <= 0) {
        return rc;
    }
	
    while (nr_bytes_sent < len) {
        rc = send(sock, buffer + nr_bytes_sent, nr_bytes_to_sent, 0);
		if(rc <= 0) return rc;
        nr_bytes_sent += rc;
        nr_bytes_to_sent -= rc;
    }
    return 1;
}

int connect_client(struct client *clients, int nr_clients, struct sockaddr_in cliaddr,
                   int connfd, struct pollfd *pfds, int *nfds, char *id_client)
{
    int client_exists = 0;

    for (int cl = 0; cl <= nr_clients; cl++)
    {
        if (strcmp(clients[cl].id, id_client) == 0)
        {
            client_exists = 1;
            if (clients[cl].is_connected)
            {
                printf("Client %s already connected.\n", id_client);
                close(connfd);
            }
            else
            {
                printf("New client %s connected from %s:%d.\n", id_client, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

                if (clients[cl].nr_msg > 1)
                {
                    // Send the stored packets to client
                    for (int idx = 0; idx < clients[cl].nr_msg; idx++)
                    {
                        int rc = send_data(clients[cl].socket, &clients[cl].messages[idx], sizeof(struct tcp_data));
                        if (rc < 0)
                        {
                            perror("write error");
                            continue;
                        }
                    }
                    clients[cl].nr_msg = 0;
                    free(clients[cl].messages);
                }

                clients[cl].is_connected = 1;
                pfds[*nfds].fd = connfd;
                pfds[*nfds].events = POLLIN;
                (*nfds)++;
            }
        }
    }
    return client_exists;
}

void send_data_to_matching_clients(struct client *clients, int nr_clients, struct tcp_data *msg)
{
    for (int idx = 0; idx < nr_clients; idx++)
    {
        for (int c_idx = 0; c_idx < clients[idx].nr_topics; c_idx++)
        {
            if (strcmp(clients[idx].topics[c_idx].name, msg->topic) == 0)
            {
                if (clients[idx].is_connected)
                {
                    // Send the message to client
                    int rc = send_data(clients[idx].socket, msg, sizeof(struct tcp_data));
                    if (rc < 0)
                    {
                        perror("write error");
                        continue;
                    }
                    // If the client is not connected and sf is 1, store the message
                }
                else if (clients[idx].topics[c_idx].sf == 1)
                {
                    if (clients[idx].nr_msg == 0)
                    {
                        clients[idx].messages = calloc(1, sizeof(struct tcp_data));
                        if (clients[idx].messages == NULL)
                        {
                            perror("calloc error");
                            break;
                        }
                        clients[idx].messages[clients[idx].nr_msg] = *msg;
                        clients[idx].nr_msg++;
                    }
                    else
                    {
                        clients[idx].messages = realloc(clients[idx].messages, (clients[idx].nr_msg + 1) * sizeof(struct tcp_data));
                        clients[idx].messages[clients[idx].nr_msg] = *msg;
                        clients[idx].nr_msg++;
                    }
                }
            }
        }
    }
}

void subscribe(struct client *clients, int cl_idx, struct tcp_message *msg_received)
{
    int flag = 0;
    for (int i = 0; i < clients[cl_idx].nr_topics; i++)
    {
        if (strcmp(clients[cl_idx].topics[i].name, msg_received->msg.name) == 0)
        {
            if (msg_received->msg.sf != clients[cl_idx].topics[i].sf)
            {
                clients[cl_idx].topics[i].sf = msg_received->msg.sf;
            }
            flag = 1;
            break;
        }
    }

    if (!flag)
    {
        if (clients[cl_idx].nr_topics == 0)
        {
            clients[cl_idx].topics = calloc(1, sizeof(struct topic));
            if (clients[cl_idx].topics == NULL)
            {
                perror("calloc error");
                return;
            }
        }
        else
        {
            clients[cl_idx].topics = realloc(clients[cl_idx].topics, (clients[cl_idx].nr_topics + 1) * sizeof(struct topic));
            if (clients[cl_idx].topics == NULL)
            {
                perror("realloc error");
                return;
            }
        }

        strcpy(clients[cl_idx].topics[clients[cl_idx].nr_topics].name, msg_received->msg.name);
        clients[cl_idx].topics[clients[cl_idx].nr_topics].sf = msg_received->msg.sf;
        clients[cl_idx].nr_topics++;
    }
}

void unsubscribe(struct client *clients, int cl_idx, struct tcp_message *msg_received)
{
    int found = 0;
    if (clients[cl_idx].nr_topics == 0)
    {
        return;
    }
    else
    {
        for (int idx = 0; idx < clients[cl_idx].nr_topics; idx++)
        {
            if (strcmp(clients[cl_idx].topics[idx].name, msg_received->msg.name) == 0)
            {
                for (int c_idx = idx; c_idx < clients[cl_idx].nr_topics - 1; c_idx++)
                    clients[cl_idx].topics[c_idx] = clients[cl_idx].topics[c_idx + 1];

                found = 1;
                break;
            }
        }

        if (found && clients[cl_idx].nr_topics > 0)
        {
            clients[cl_idx].topics = realloc(clients[cl_idx].topics, (clients[cl_idx].nr_topics - 1) * sizeof(struct topic));
            clients[cl_idx].nr_topics--;
            if (clients[cl_idx].topics == NULL && clients[cl_idx].nr_topics != 0)
            {
                perror("realloc error");
                return;
            }
        }
        else if (found)
        {
            free(clients[cl_idx].topics);
            clients[cl_idx].nr_topics--;
        }
    }
}