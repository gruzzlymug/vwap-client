#pragma once
#include <cstdint>

#include <stddef.h>

struct Header {
    unsigned char length;
    unsigned char message_type;
};

struct Quote {
    uint64_t timestamp;
    uint64_t symbol;
    int32_t bid_price_c;
    uint32_t bid_qty;
    int32_t ask_price_c;
    uint32_t ask_qty;
};

class Trade {
public:
    uint64_t timestamp;
    uint64_t symbol;
    int32_t price_c;
    uint32_t qty;

    Trade() : timestamp(0), symbol(0), price_c(0), qty(0) {
    }

    void* operator new (size_t, void* buffer) {
        return buffer;
    }

    void operator delete(void*, void*) {
    }
};

struct Order {
    uint64_t timestamp;
    uint64_t symbol;
    int32_t price_c;
    uint32_t qty;
    char side;
};
