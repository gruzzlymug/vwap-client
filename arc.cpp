#include "arc.h"

#include <algorithm>
#include <chrono>
#include <thread>

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

using namespace std::chrono;

ArcConfig Arc::config;
struct sockaddr_in Arc::serv_addr;
int64_t Arc::vwap = INTMAX_MAX;
std::vector<Trade> Arc::trades;

Arc::Arc() {
}

Arc::~Arc() {
}

int Arc::start(ArcConfig *new_config) {
    //printf("%s:%d\n", config.market_server_ip, config.market_server_port);
    //printf("%s:%d\n", config.order_server_ip, config.order_server_port);
    memcpy(&config, new_config, sizeof(ArcConfig));

    int market_socket = connect(config.market_server_ip, config.market_server_port);
    int order_socket = connect(config.order_server_ip, config.order_server_port);
    std::thread t1(stream_market_data, market_socket);
    std::thread t2(send_order_data, order_socket);
    std::thread t3(calc_vwap);
    t3.join();
    t2.join();
    t1.join();
    close(order_socket);
    close(market_socket);
    return 0;
}

int Arc::send_order_data(int socket) {
    char buffer[256];
    bzero(buffer, 256);

    printf("piping order data\n");
    if (vwap < 0) {
        Order order;
        unsigned char bytes_sent = sizeof(order);
        memcpy(buffer, &order, sizeof(order));
        int n = write(socket, buffer, bytes_sent);
        if (n < 0) {
        }

        read_bytes(socket, 32, buffer);
        memcpy(&order, buffer, sizeof(order));
        // TODO: deal with 7 char ticker
        printf("o> %" PRIu64 " %7s %c $%d x %d\n", order.timestamp, order.symbol, order.side, order.price_c, order.qty);
    }
    return 0;
}

int Arc::calc_vwap() {
    while (true) {
        uint64_t now_ns = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
        uint64_t period_ns = config.vwap_period_s * 1000000000;
        int64_t total_spent = 0;
        int64_t total_contracts = 0;
        uint64_t cutoff_ns = now_ns - period_ns;
        for (auto t : trades) {
            if (t.timestamp > cutoff_ns) {
                total_spent += t.price_c * t.qty * 100;
                total_contracts += t.qty;
                //printf("v> %lx %lx %7s %8d %4d\n", now_ns, t.timestamp, t.symbol, t.price_c, t.qty);
            } else {
                //printf("_> %lx %lx %7s %8d %4d\n", now_ns, t.timestamp, t.symbol, t.price_c, t.qty);
            }
        }
        if (total_contracts > 0) {
            vwap = total_spent / total_contracts;
        }
        printf("vwap = %" PRIu64 "\n", vwap);
        sleep(1);
    }
}

int Arc::stream_market_data(int socket) {
    char buffer[256];
    bzero(buffer, 256);

    while (true) {
        read_bytes(socket, 2, buffer);
        unsigned int length = (unsigned char) buffer[0];
        unsigned int message_type = (unsigned char) buffer[1];
        //printf("-> %d %d\n", length, message_type);
        switch (message_type) {
        case 1: {
            Quote quote;
            read_bytes(socket, length, buffer);
            memcpy(&quote, buffer, sizeof(quote));
            if (strncmp(quote.symbol, config.symbol, strlen(config.symbol)) == 0) {
                printf("q> %" PRIx64 " %7s $%8d x %3d, $%8d x %3d\n", quote.timestamp, quote.symbol, quote.bid_price_c, quote.bid_qty, quote.ask_price_c, quote.ask_qty);
                if (config.side == 'B' && (quote.ask_price_c * 100 <= vwap)) {
                    //printf("BUY\n");
                } else if (config.side == 'S' && (quote.bid_price_c * 100 >= vwap)) {
                    //printf("SELL\n");
                }
            }
            break;
        }
        case 2: {
            Trade trade;
            read_bytes(socket, length, buffer);
            memcpy(&trade, buffer, sizeof(trade));
            if (strncmp(trade.symbol, config.symbol, strlen(trade.symbol)) == 0) {
                // TODO: emplace_back, custom allocator
                trades.push_back(trade);
            }

            uint64_t now_ns = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
            uint64_t period_ns = config.vwap_period_s * 1000000000;
            uint64_t cutoff_ns = now_ns - period_ns;
            trades.erase(std::remove_if(trades.begin(), trades.end(), [cutoff_ns](Trade &t) { return t.timestamp < cutoff_ns; }), trades.end());
            printf("t> %" PRIx64 " %7s $%8d x %3d\n", trade.timestamp, trade.symbol, trade.price_c, trade.qty);
            break;
        }
        default:
            printf("PROBLEM\n");
        }
    }

    return 0;
}

int Arc::connect(char *hostname, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        return -1;
    }

    printf("Connecting to %s\n", hostname);
    // TODO: understand management of *server memory
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
    return sockfd;
}

int Arc::send(char *message) {
    //int n = write(sockfd, message, strlen(message));
    //if (n < 0) {
    //	perror("ERROR writing to socket");
    //	return -1;
    //}
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
