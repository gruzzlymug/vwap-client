#pragma once
#include <cstdint>

struct Header {
    unsigned char length;
    unsigned char message_type;
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
    uint64_t symbol;
    int32_t price_c;
    uint32_t qty;

    Trade(uint64_t t, uint64_t s, int32_t p, uint32_t q)
        : timestamp(t), symbol(s), price_c(p), qty(q)
    {
    }
    Trade& operator=(const Trade& other) = default;
};

struct Order {
    uint64_t timestamp;
    char symbol[8];
    int32_t price_c;
    uint32_t qty;
    char side;
};
