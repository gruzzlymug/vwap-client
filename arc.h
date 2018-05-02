#pragma once

#include <netinet/in.h>

class Arc {
	int sockfd;
	struct sockaddr_in serv_addr;

public:
	Arc();
	~Arc();
	int connect(char *hostname, int port);
	int send(char *message);
	int receive();
};

