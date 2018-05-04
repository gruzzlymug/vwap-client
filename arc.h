#pragma once
#include "market_types.h"

#include <vector>
#include <netinet/in.h>

class Arc {
	int sockfd;
	static struct sockaddr_in serv_addr;
	static std::vector<Trade> trades;

	static int read_bytes(int socket, unsigned int num_to_read, char *buffer);

public:
	Arc();
	~Arc();
	int start(char *hostname, int port, int order_port);
	static int pipe_market_data(int socket);
	static int pipe_order_data(int socket);
	int connect(char *hostname, int port);
	int send(char *message);
	int receive();
};

