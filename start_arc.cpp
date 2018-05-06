#include "arc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "usage %s hostname port port\n", argv[0]);
        exit(0);
    }

    ArcConfig config;
    bzero(&config, sizeof(config));

    strncpy(config.symbol, "BTC.USD", sizeof("BTC.USD"));
    config.side = 'S';
    config.qty_max = 4;
    config.vwap_period_s = 30;
    config.order_timeout_s = 5;
    strncpy(config.market_server_ip, argv[1], strlen(argv[1]));
    config.market_server_port = atoi(argv[2]);
    strncpy(config.order_server_ip, argv[1], strlen(argv[1]));
    config.order_server_port = atoi(argv[3]);

    Arc arc;
    arc.start(&config);

    return 0;
}
