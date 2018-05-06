#pragma once
#include <cstdint>

struct Header {
    unsigned char length;
    unsigned char message_type;
};

struct Common {
    uint64_t timestamp;
    char symbol[8];
    int32_t price_c;
    uint32_t qty;
};

struct Quote {
    uint64_t timestamp;
    char symbol[8];
    int32_t bid_price_c;
    uint32_t bid_qty;
    int32_t ask_price_c;
    uint32_t ask_qty;
};

struct Trade {
    uint64_t timestamp;
    char symbol[8];
    int32_t price_c;
    uint32_t qty;
};

struct Order {
    uint64_t timestamp;
    char symbol[8];
    int32_t price_c;
    uint32_t qty;
    char side;
};
