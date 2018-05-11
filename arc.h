#pragma once
#include "market_types.h"

#include <atomic>
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
    enum class State { INIT, RUN, QUIT };

    static ArcConfig config_;
    static struct sockaddr_in serv_addr_;
    static int64_t vwap_;
    static std::atomic<State> state_;

    static const int max_trades = 32;
    static Trade all_trades[max_trades * 2];
    static uint16_t t_first;
    static uint16_t t_next;

    static const int max_orders = 32;
    static Order all_orders[max_orders * 2];
    static uint16_t o_first;
    static uint16_t o_next;

    static uint64_t last_order_ns;

    static int stream_market_data(int socket);
    static int stream_quote(char *buffer, size_t length);
    static int stream_trade(char *buffer, size_t length);

    static int calc_vwap();
    static int send_order_data(int socket);
    static int handle_kb();

    static int connect(char *hostname, int port);
    static int read_bytes(int socket, unsigned int num_to_read, char *buffer);

public:
    Arc();
    ~Arc();
    int start(ArcConfig *config);
};
