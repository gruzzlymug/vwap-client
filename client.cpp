#include "client.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

Client::Client()
	: sockfd(-1) {
}

Client::~Client() {
	if (sockfd >= 0) {
		close(sockfd);
	}
}

int Client::connect(char *hostname, int port) {
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("ERROR opening socket");
		return -1;
	}

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
	return 0;
}

int Client::send(char *message) {
	int n = write(sockfd, message, strlen(message));
	if (n < 0) {
		perror("ERROR writing to socket");
		return -1;
	}
	return 0;
}

int Client::receive() {
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

