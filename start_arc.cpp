#include "arc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "usage %s hostname port\n", argv[0]);
		exit(0);
	}

	Arc arc;

	int portno = atoi(argv[2]);
	//arc.pipe_market_data(argv[1], portno);
	arc.start(argv[1], portno);

	//char buffer[256];
	//bzero(buffer, 256);
	//while (strncmp("QUIT", buffer, strlen("QUIT")) != 0) {
	//	printf("Enter the message: ");
		//bzero(buffer, 256);
		//fgets(buffer, 255, stdin);
		
		//arc.send(buffer);
	//	arc.receive();
	//}

	return 0;
}
