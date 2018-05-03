#pragma once

#include <netinet/in.h>

class Arc {
	int sockfd;
	static struct sockaddr_in serv_addr;

	static int read_bytes(int socket, unsigned int num_to_read, char *buffer);

public:
	Arc();
	~Arc();
	int start(char *hostname, int port);
	static int pipe_market_data(char *hostname, int port);
	int connect(char *hostname, int port);
	int send(char *message);
	int receive();
};

