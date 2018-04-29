#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
