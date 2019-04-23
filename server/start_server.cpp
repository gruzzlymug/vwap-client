#include "server.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void int_handler(int s) {
    printf("Caught signal %d!!!\n",s);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "ERROR, no port or mode provided\n");
        exit(1);
    }

    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = int_handler;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;

    //sigaction(SIGINT, &sig_int_handler, NULL);

    int mode = atoi(argv[2]);
    Server server(mode);

    int portno = atoi(argv[1]);
    server.connect(portno);

    server.listen();

    return 0;
}
