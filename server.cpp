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
				send_quotes(newsockfd);
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
	std::uniform_int_distribution<> price_range(-100, 100);
	std::uniform_int_distribution<> contract_range(1, 13);
	std::uniform_int_distribution<> delay_range(1, 100);
	std::uniform_int_distribution<> symbol_range(0, 2);

	Trade t;
	while (true) {
		uint64_t nanoseconds_since_epoch = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
		//printf("@ (%lx) %ld\n", nanoseconds_since_epoch, nanoseconds_since_epoch);
		t.timestamp = nanoseconds_since_epoch;
		bzero(t.symbol, 8);
		switch (symbol_range(gen)) {
			case 0:
				strncpy(t.symbol, "BTC.USD", strlen("BTC.USD")) ;
				t.price_c = 991330 + price_range(gen) * 2;
				break;
			case 1:
				strncpy(t.symbol, "ETH.USD", strlen("ETH.USD")) ;
				t.price_c = 82050 + price_range(gen);
				break;
			default:
				strncpy(t.symbol, "XYZ.USD", strlen("XYZ.USD")) ;
				t.price_c = 1000 + price_range(gen);
		}
		t.qty = contract_range(gen);

		send_trade(t, socket);

		unsigned int delay_us = delay_range(gen) * 1000;
		usleep(delay_us);
	}
}

/*
void Server::send_orders(int socket) {
    char buffer[256];
    bzero(buffer, 256);

    printf("piping order data\n");
    while (true) {
        Order order;
        read_bytes(socket, 32, buffer);
        memcpy(&order, buffer, sizeof(order));
	    printf("o> %lx %7s %c $%d x %d\n", order.timestamp, order.symbol, order.side, order.price_c, order.qty);
	}
}
*/

int Server::read_bytes(int socket, unsigned int num_to_read, char *buffer) {
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

void Server::send_quotes(int socket) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> price_range(1, 60);
	std::uniform_int_distribution<> contract_range(1, 13);
	std::uniform_int_distribution<> side_range(0, 1);
	std::uniform_int_distribution<> delay_range(1, 100);

	unsigned char spread = 10;

	Quote q;
	while (true) {
		uint64_t now_ns = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
		q.timestamp = now_ns;
		bzero(q.symbol, 8);
		strncpy(q.symbol, "BTC.USD", strlen("BTC.USD"));
		q.bid_price_c = 991330 + price_range(gen) - spread;
		q.bid_qty = contract_range(gen);
		q.ask_price_c = 991330 + price_range(gen) + spread;
		q.ask_qty = contract_range(gen);

		send_quote(q, socket);

		unsigned int delay_us = delay_range(gen) * 50000;
		usleep(delay_us);
	}
}

void Server::send_quote(Quote quote, int socket) {
	char buffer[256];
	unsigned char quote_size = sizeof(quote);

	printf("Sending Quote\n");
	buffer[0] = quote_size;
	buffer[1] = 0x01;
	int n = write(socket, buffer, 2);
	if (n < 0) {
	}
	memcpy(buffer, &quote, quote_size);
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
	memcpy(buffer, &trade, sizeof(trade));
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

