#include "server.h"

#include <chrono>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

using namespace std::chrono;

Server::Server(int mode)
	: sockfd(-1), mode(mode), done(false) {
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
		printf("accepting\n");
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
			switch (mode) {
			case 0:
				send_market_data(newsockfd);
				break;
			case 1:
				send_orders(newsockfd);
				break;
			default:
				printf("BAD MODE %d\n", mode);
			}
			exit(0);
		} else {
			close(newsockfd);
		}
	}
}

void Server::stop() {
	done = true;
}


void Server::send_market_data(int socket) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> price_range(1, 60);
	std::uniform_int_distribution<> contract_range(1, 13);
	std::uniform_int_distribution<> delay_range(1, 100);

	Trade t;
	while (true) {
		uint64_t nanoseconds_since_epoch = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
		//printf("@ (%lx) %ld\n", nanoseconds_since_epoch, nanoseconds_since_epoch);
		t.timestamp = nanoseconds_since_epoch;
		bzero(t.symbol, 8);
		strncpy(t.symbol, "BTC.USD", strlen("BTC.USD")) ;
		t.price_c = 93 + price_range(gen);
		t.qty = contract_range(gen);

		send_trade(t, socket);

		unsigned int delay_us = delay_range(gen) * 10000;
		usleep(delay_us);
	}
}

void Server::send_orders(int socket) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> price_range(1, 60);
	std::uniform_int_distribution<> contract_range(1, 13);
	std::uniform_int_distribution<> side_range(0, 1);
	std::uniform_int_distribution<> delay_range(1, 100);

	Order o;
	while (true) {
		uint64_t nanoseconds_since_epoch = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
		o.timestamp = nanoseconds_since_epoch;
		bzero(o.symbol, 8);
		strncpy(o.symbol, "BTC.USD", strlen("BTC.USD"));
		o.price_c = 73 + price_range(gen);
		o.qty = contract_range(gen);
		o.side = side_range(gen) == 0 ? 'B' : 'S';

		send_order(o, socket);

		unsigned int delay_us = delay_range(gen) * 500000;
		usleep(delay_us);
	}
}

void Server::send_quote(Quote quote, int socket) {
	char buffer[256];
	printf("Sending Quote\n");
	buffer[0] = 0x04;
	buffer[1] = 0x01;
	int n = write(socket, buffer, 2);
	if (n < 0) {
	}
	buffer[0] = 0xde;
	buffer[1] = 0xad;
	buffer[2] = 0xbe;
	buffer[3] = 0xef;
	int p = write(socket, buffer, 4);
	if (p < 0) {
	}
}

// TODO: send pointer
void Server::send_trade(Trade trade, int socket) {
	char buffer[64];
	unsigned char bytes_sent = sizeof(trade);

	printf("Sending Trade\n");
	buffer[0] = bytes_sent;
	buffer[1] = 0x02;
	int n = write(socket, buffer, 2);
	if (n < 0) {
	}
	// TODO: check and send with proper endianness
	//printf("t: %lx\n", trade.timestamp);
	memcpy(buffer, &trade, sizeof(trade));
	//buffer[1] = trade.timestamp & 0xff00;
	//buffer[2] = trade.timestamp & 0xff00);
	//buffer[3] = trade.timestamp & 0xff0000);
	//buffer[4] = trade.timestamp & 0xff000000);
	//buffer[5] = trade.timestamp & 0xff00000000);
	//buffer[6] = trade.timestamp & 0xff0000000000);
	//buffer[7] = trade.timestamp & 0xff000000000000);
	int p = write(socket, buffer, bytes_sent);
	if (p < 0) {
	}
}

// TODO: send pointer
void Server::send_order(Order order, int socket) {
	char buffer[256];

	printf("Sending Order\n");
	unsigned char bytes_sent = sizeof(order);
	memcpy(buffer, &order, sizeof(order));
	int n = write(socket, buffer, bytes_sent);
	if (n < 0) {
	}
}

