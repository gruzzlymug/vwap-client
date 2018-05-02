#include "server.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

//	printf("Enter the message: ");
//	char buffer[256];
//	bzero(buffer, 256);
//	fgets(buffer, 255, stdin);
//	server.send(buffer);
//	server.receive();

