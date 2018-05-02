struct MdHeader {
	unsigned char length;
	unsigned char message_type;
};

struct MdQuote {
	unsigned long timestamp;
	char symbol[8];
	int bid_price_c;
	unsigned int bid_qty;
	int ask_price_c;
	unsigned int ask_qty;
};

struct MdTrade {
	unsigned long timestamp;
	char symbol[8];
	int price_c;
	unsigned int qty;
};

