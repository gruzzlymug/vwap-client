#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

void dostuff(int newsockfd);

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

bool done = false;

void int_handler(int s) {
	printf("Caught signal %d\n",s);
	done = true;
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

	sigaction(SIGINT, &sig_int_handler, NULL);

	signal(SIGCHLD, SIG_IGN);

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		error("ERROR opening socket");
	}

	struct sockaddr_in serv_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));
	int portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		error("ERROR on binding");
	}
	listen(sockfd, 5);

	struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);

	while (!done) {
		int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) {
			error("ERROR on accept");
		}
		printf("-\n");
		int pid = fork();
		if (pid < 0) {
			error("ERROR on fork");
		}
		if (pid == 0) {
			close(sockfd);
			dostuff(newsockfd);
			exit(0);
		} else {
			close(newsockfd);
		}
	}

	close(sockfd);

	return 0;
}

void dostuff(int newsockfd) {
	char buffer[256];
	bzero(buffer, 256);

	int n = read(newsockfd, buffer, 255);
	if (n < 0) {
		error("ERROR reading from socket");
	}

	printf("Here is the message: %s\n", buffer);
	n = write(newsockfd, "Received", sizeof("Received"));
	if (n < 0) {
		error("ERROR writing to socket");
	}

	close(newsockfd);
}

