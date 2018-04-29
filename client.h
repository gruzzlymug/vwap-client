class Client {
	int sockfd;
	struct sockaddr_in serv_addr;

public:
	Client();
	~Client();
	int connect(char *hostname, int port);
	int send(char *message);
	int receive();
};

