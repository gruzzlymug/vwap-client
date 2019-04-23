#pragma once

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
