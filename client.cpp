#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
//#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include "client.h"

void error(const char *msg) {
	perror(msg);
	exit(0);
}

Client::Client() {
	sockfd = -1;
}

Client::~Client() {
	if (sockfd >= 0) {
		close(sockfd);
	}
}

int Client::connect(char *hostname, int port) {
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		//error("ERROR opening socket");
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
		//error("ERROR connecting");
		return -1;
	}
	return 0;
}

int Client::send(char *message) {
	int n = write(sockfd, message, strlen(message));
	if (n < 0) {
		//error("ERROR writing to socket");
	}
	return 0;
}

int Client::receive() {
	char buffer[256];
	bzero(buffer, 256);
	int n = read(sockfd, buffer, 255);
	if (n < 0) {
		//error("ERROR reading from socket");
	}
	printf("%s\n", buffer);
	return 0;
}

int main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "usage %s hostname port\n", argv[0]);
		exit(0);
	}

	Client client;

	int portno = atoi(argv[2]);
	client.connect(argv[1], portno);

	printf("Enter the message: ");
	char buffer[256];
	bzero(buffer, 256);
	fgets(buffer, 255, stdin);

	client.send(buffer);

	client.receive();

	return 0;
}

