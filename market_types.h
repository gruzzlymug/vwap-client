#pragma once
#include <cstdint>

struct Header {
	unsigned char length;
	unsigned char message_type;
};

struct Common {
	unsigned long timestamp;
	char symbol[8];
	int price_c;
	unsigned int qty;
};

struct Quote {
	unsigned long timestamp;
	char symbol[8];
	int bid_price_c;
	unsigned int bid_qty;
	int ask_price_c;
	unsigned int ask_qty;
};

struct Trade {
	unsigned long timestamp;
	char symbol[8];
	int price_c;
	unsigned int qty;
};

struct Order {
	uint64_t timestamp;
	char symbol[8];
	int32_t price_c;
	uint32_t qty;
	char side[1];
};
