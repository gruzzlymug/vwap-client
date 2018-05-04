#include "market_types.h"

class Server {
	int sockfd;
	int mode;
	bool done;

	void send_quote(Quote quote, int socket);
	void send_trade(Trade trade, int socket);
	void send_orders(int socket);

	void dostuff(int newsockfd);
	//void dostuff_old(int newsockfd);

public:
	Server(int mode);
	~Server();

	void connect(int port);
	void listen();
	void stop();
};

