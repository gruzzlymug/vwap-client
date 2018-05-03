#include "arc.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <thread>

struct sockaddr_in Arc::serv_addr;

Arc::Arc()
	: sockfd(-1) {
}

Arc::~Arc() {
	if (sockfd >= 0) {
		close(sockfd);
	}
}

int Arc::start(char *hostname, int port) {
	std::thread t1(pipe_market_data, hostname, port);
	t1.join();
	return 0;
}

int Arc::pipe_market_data(char *hostname, int port) {
	int socket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (socket < 0) {
		perror("ERROR opening socket");
		return -1;
	}

	printf("Connecting to %s\n", hostname);
	struct hostent *server = gethostbyname(hostname);
	if (server == NULL) {
		fprintf(stderr, "ERROR no such host\n");
		return -1;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(port);
	if (::connect(socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR connecting");
		return -1;
	}

	char buffer[256];
	bzero(buffer, 256);

	while (true) {
		read_bytes(socket, 2, buffer);
		unsigned int length = (unsigned char) buffer[0];
		unsigned int message_type = (unsigned char) buffer[1];
		printf("-> %d %d\n", length, message_type);
		read_bytes(socket, length, buffer);
		printf("b> %x\n", (unsigned char) buffer[0]);
		printf("b> %x\n", (unsigned char) buffer[1]);
		printf("b> %x\n", (unsigned char) buffer[2]);
		printf("b> %x\n", (unsigned char) buffer[3]);
	}

	close(socket);

	return 0;
}

int Arc::connect(char *hostname, int port) {
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

