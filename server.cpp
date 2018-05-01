#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

// TODO ? perror followed by exit(1);
class Server {
	int sockfd;
	bool done;

	void dostuff(int newsockfd);

public:
	Server();
	~Server();

	void connect(int port);
	void listen();

	void stop();
};

Server::Server()
	: sockfd(-1), done(false) {
	signal(SIGCHLD, SIG_IGN);
}

Server::~Server() {
	close(sockfd);
}

void Server::connect(int port) {
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("ERROR opening socket");
		return;
	}

	struct sockaddr_in serv_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR on binding");
		return;
	}
	::listen(sockfd, 5);
}

void Server::listen() {
	struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);

	while (!done) {
		int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) {
			perror("ERROR on accept");
			return;
		}
		printf("-\n");
		int pid = fork();
		if (pid < 0) {
			perror("ERROR on fork");
			return;
		}
		if (pid == 0) {
			close(sockfd);
			dostuff(newsockfd);
			exit(0);
		} else {
			close(newsockfd);
		}
	}
}

void Server::stop() {
	done = true;
}

void Server::dostuff(int newsockfd) {
	char buffer[256];
	bzero(buffer, 256);

	int n = read(newsockfd, buffer, 255);
	if (n < 0) {
		perror("ERROR reading from socket");
	}

	printf("Here is the message: %s\n", buffer);
	n = write(newsockfd, "Received", sizeof("Received"));
	if (n < 0) {
		perror("ERROR writing to socket");
	}

	// TODO use goto to get here from errors?
	// TODO track orphaned socket if returning
	close(newsockfd);
}

void int_handler(int s) {
	printf("Caught signal %d!!!\n",s);
}

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "ERROR, no port provided\n");
		exit(1);
	}

	struct sigaction sig_int_handler;
	sig_int_handler.sa_handler = int_handler;
	sigemptyset(&sig_int_handler.sa_mask);
	sig_int_handler.sa_flags = 0;

	//sigaction(SIGINT, &sig_int_handler, NULL);

	Server server;

	int portno = atoi(argv[1]);
	server.connect(portno);

	server.listen();

	return 0;
}

