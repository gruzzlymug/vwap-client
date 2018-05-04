#include "market_types.h"

class Server {
	int sockfd;
	int mode;
	bool done;

	void send_market_data(int socket);
	void send_orders(int socket);

	void send_quote(Quote quote, int socket);
	void send_trade(Trade trade, int socket);
	void send_order(Order order, int socket);

public:
	Server(int mode);
	~Server();

	void connect(int port);
	void listen();
	void stop();
};

