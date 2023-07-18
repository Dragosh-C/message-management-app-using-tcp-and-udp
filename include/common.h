#ifndef __COMMON_H_
#define __COMMON_H_

#include <stdint.h>

#define MAXSIZE 1500
#define PORT 8313

// struct seq_udp {
//   uint32_t len;
//   uint16_t seq;
//   char payload[MAXSIZE];
// };

struct topic{
	char name[100];
	int sf;
};

struct udp_message {
	char topic[50];
	uint8_t type;
	char buff[1500];
};

struct tcp_message {
	char client_id[11];
	char action[12];
	struct topic msg;
};

struct tcp_data {

	char ip[16];
	uint16_t port;
	char topic[51];
	char type[11];
	char buff[1501];
};

struct client {
	char id[11]; 
	int socket;
	struct topic *topics;
	int nr_topics;
	int is_connected;
	struct tcp_data *messages; // mesajele stocate
	int nr_msg;
};


#endif
