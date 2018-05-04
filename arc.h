#pragma once
#include "market_types.h"

#include <vector>
#include <netinet/in.h>

struct ArcConfig {
	char symbol[8];
	char side;
    unsigned int max_qty;
    unsigned int vwap_period;
    unsigned int order_timeout;
    // TODO: unify ip:port
    char market_server_ip[32];
    unsigned int market_server_port;
    char order_server_ip[32];
    unsigned int order_server_port;
};

class Arc {
	int sockfd;
	static struct sockaddr_in serv_addr;
	static std::vector<Trade> trades;

	static int read_bytes(int socket, unsigned int num_to_read, char *buffer);

public:
	Arc();
	~Arc();
	int start(ArcConfig& config);
	static int pipe_market_data(int socket);
	static int pipe_order_data(int socket);
	static int calc_vwap();
	int connect(char *hostname, int port);
	int send(char *message);
	int receive();
};

