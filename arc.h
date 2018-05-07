#pragma once
#include "market_types.h"

#include <vector>

struct ArcConfig {
    char symbol[8];
    char side;
    unsigned int qty_max;
    unsigned int vwap_period_s;
    unsigned int order_timeout_s;
    // TODO: unify ip:port
    char market_server_ip[32];
    unsigned int market_server_port;
    char order_server_ip[32];
    unsigned int order_server_port;
};

class Arc {
    static ArcConfig config;
    static bool initializing;
    static struct sockaddr_in serv_addr;
    static int64_t vwap;
    static std::vector<Trade> trades;
    static std::vector<Order> orders;

    static int stream_market_data(int socket);
    static int calc_vwap();
    static int send_order_data(int socket);

    static int connect(char *hostname, int port);
    static int read_bytes(int socket, unsigned int num_to_read, char *buffer);

public:
    Arc();
    ~Arc();
    int start(ArcConfig *new_config);
};
