#include "arc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "usage %s hostname port\n", argv[0]);
		exit(0);
	}

	int portno = atoi(argv[2]);
	int order_port = atoi(argv[3]);
	
	Arc arc;
	arc.start(argv[1], portno, order_port);

	return 0;
}
