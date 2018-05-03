class Server {
	int sockfd;
	bool done;

	void dostuff(int newsockfd);
	//void dostuff_old(int newsockfd);

public:
	Server();
	~Server();

	void connect(int port);
	void listen();
	void stop();
};

