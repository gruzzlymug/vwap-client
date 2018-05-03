class Server {
	int sockfd;
	int mode;
	bool done;

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

