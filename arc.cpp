#include "arc.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <thread>

struct sockaddr_in Arc::serv_addr;
std::vector<Trade> Arc::trades;

Arc::Arc()
	: sockfd(-1) {
}

Arc::~Arc() {
	if (sockfd >= 0) {
		close(sockfd);
	}
}

int Arc::start(ArcConfig& config) {
	//printf("%s:%d\n", config.market_server_ip, config.market_server_port);
	//printf("%s:%d\n", config.order_server_ip, config.order_server_port);

	int market_socket = connect(config.market_server_ip, config.market_server_port);
	int order_socket = connect(config.order_server_ip, config.order_server_port);
	std::thread t1(pipe_market_data, market_socket);
	std::thread t2(pipe_order_data, order_socket);
	std::thread t3(calc_vwap);
	t3.join();
	t2.join();
	t1.join();
	close(order_socket);
	close(market_socket);
	return 0;
}

int Arc::pipe_market_data(int socket) {
	char buffer[256];
	bzero(buffer, 256);

	while (true) {
		read_bytes(socket, 2, buffer);
		unsigned int length = (unsigned char) buffer[0];
		unsigned int message_type = (unsigned char) buffer[1];
		switch (message_type) {
		case 1: {
			printf("-> %d %d\n", length, message_type);
			read_bytes(socket, length, buffer);
			printf("q> %x\n", (unsigned char) buffer[0]);
			printf("q> %x\n", (unsigned char) buffer[1]);
			printf("q> %x\n", (unsigned char) buffer[2]);
			printf("q> %x\n", (unsigned char) buffer[3]);
			break;
		}
		case 2: {
			Trade trade;
			printf("-> %d %d\n", length, message_type);
			read_bytes(socket, length, buffer);
			memcpy(&trade, buffer, sizeof(trade));
			trades.push_back(trade);
			printf("t> %lx %7s $%d x %d\n", trade.timestamp, trade.symbol, trade.price_c, trade.qty);
			/*
			printf("t> %x\n", (unsigned char) buffer[0]);
			printf("t> %x\n", (unsigned char) buffer[1]);
			printf("t> %x\n", (unsigned char) buffer[2]);
			printf("t> %x\n", (unsigned char) buffer[3]);
			printf("t> %x\n", (unsigned char) buffer[4]);
			printf("t> %x\n", (unsigned char) buffer[5]);
			printf("t> %x\n", (unsigned char) buffer[6]);
			printf("t> %x\n", (unsigned char) buffer[7]);
			*/
			break;
		}
		default:
			printf("PROBLEM\n");
		}
	}

	return 0;
}

int Arc::pipe_order_data(int socket) {
	char buffer[256];
	bzero(buffer, 256);

	printf("order: %d\n", (int) sizeof(Order));
	while (true) {
		read_bytes(socket, 2, buffer);
		unsigned int length = (unsigned char) buffer[0];
		unsigned int message_type = (unsigned char) buffer[1];
		printf("-> %d %d\n", length, message_type);
		read_bytes(socket, length, buffer);
		printf("o> %x\n", (unsigned char) buffer[0]);
		printf("o> %x\n", (unsigned char) buffer[1]);
		printf("o> %x\n", (unsigned char) buffer[2]);
		printf("o> %x\n", (unsigned char) buffer[3]);
	}
	return 0;
}

int Arc::calc_vwap() {
	while (true) {
		printf("tc: %lu\n", trades.size());
		sleep(1);
	}
}

int Arc::connect(char *hostname, int port) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("ERROR opening socket");
		return -1;
	}

	printf("Connecting to %s\n", hostname);
	// TODO: understand management of *server memory
	struct hostent *server = gethostbyname(hostname);
	if (server == NULL) {
		fprintf(stderr, "ERROR no such host\n");
		return -1;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(port);
	if (::connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR connecting");
		return -1;
	}
	return sockfd;
}

int Arc::send(char *message) {
	int n = write(sockfd, message, strlen(message));
	if (n < 0) {
		perror("ERROR writing to socket");
		return -1;
	}
	return 0;
}

int Arc::read_bytes(int socket, unsigned int num_to_read, char *buffer) {
	unsigned int bytes_read = 0;
	while (bytes_read < num_to_read) {
		int n = read(socket, buffer, num_to_read);
		if (n < 0) {
			perror("ERROR reading from socket");
			return -1;
		}
		bytes_read += n;
	}
	return 0;
}

int Arc::receive() {
	char buffer[256];
	bzero(buffer, 256);
	int n = read(sockfd, buffer, 255);
	if (n < 0) {
		perror("ERROR reading from socket");
		return -1;
	}
	printf("%s\n", buffer);
	return 0;
}

