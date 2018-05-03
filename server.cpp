#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

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
				dostuff(newsockfd);
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

void Server::dostuff(int newsockfd) {
	char buffer[256];
	while (true) {
		printf("Sending Quote\n");
		buffer[0] = 0x04;
		buffer[1] = 0x01;
		int n = write(newsockfd, buffer, 2);
		if (n < 0) {
		}
		buffer[0] = 0xde;
		buffer[1] = 0xad;
		buffer[2] = 0xbe;
		buffer[3] = 0xef;
		int p = write(newsockfd, buffer, 4);
		if (p < 0) {
		}
		sleep(9);
	}
}

void Server::send_orders(int socket) {
	char buffer[256];
	while (true) {
		printf("Sending Order\n");
		buffer[0] = 0x04;
		buffer[1] = 0x01;
		int n = write(socket, buffer, 2);
		if (n < 0) {
		}
		buffer[0] = 0xea;
		buffer[1] = 0xea;
		buffer[2] = 0xea;
		buffer[3] = 0xea;
		int p = write(socket, buffer, 4);
		if (p < 0) {
		}
		sleep(3);
	}
}

/*
void Server::dostuff_old(int newsockfd) {
	char buffer[256];
	bzero(buffer, 256);

  while (true) {
	int n = read(newsockfd, buffer, 255);
	if (n < 0) {
		perror("ERROR reading from socket");
	}

	if (strncmp("QUIT", buffer, strlen("QUIT")) == 0) {
		break;
	}

	printf("Request: %s\n", buffer);
	if (strncmp("QUOTE", buffer, strlen("QUOTE")) == 0) {
		printf("Sending Quote\n");
		n = write(newsockfd, "A QUOTE", sizeof("A QUOTE"));
	} else if (strncmp("TRADE", buffer, strlen("TRADE")) == 0) {
		printf("Sending Trade\n");
		n = write(newsockfd, "A TRADE", sizeof("A TRADE"));
	} else if (strncmp("ORDER", buffer, strlen("ORDER")) == 0) {
		printf("Sending Order\n");
		n = write(newsockfd, "AN ORDER", sizeof("AN ORDER"));
	} else {
		n = write(newsockfd, "Received", sizeof("Received"));
	}
	if (n < 0) {
		perror("ERROR writing to socket");
	}
  }
  printf("Bailing...\n");
	// TODO use goto to get here from errors?
	// TODO track orphaned socket if returning
	close(newsockfd);
}
*/
