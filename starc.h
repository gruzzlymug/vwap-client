#pragma once
#include "arc_config.h"
#include "market_types.h"

#include <atomic>

#include <netinet/in.h>
#include <stddef.h>

class Starc {
    enum class State { INIT, RUN, QUIT };

    static constexpr int MAX_TRADES = 32;

    int connect(char *hostname, int port);
    int stream_market_data(int market_socket, int order_socket);
    int read_bytes(int socket, unsigned int num_to_read, char *buffer);

    bool deserialize_trade(char *buffer, size_t length);
    void trim_trades();
    void calc_vwap();

    bool deserialize_quote(char *buffer, size_t length);
    bool ready_to_order();
    int place_order(int socket);
    void serialize_order(int socket, const Order& order);

    char buffer_[256];

    ArcConfig config_;
    std::atomic<State> state_;
    struct sockaddr_in serv_addr_;
    int64_t vwap_;

    Trade all_trades_[MAX_TRADES * 2];
    uint16_t t_first_;
    uint16_t t_next_;

    Quote quote_;
    uint64_t last_order_ns_;

public:
    Starc();
    int run(ArcConfig *config);
};
