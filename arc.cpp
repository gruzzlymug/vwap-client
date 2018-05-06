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

#ifdef __APPLE__
#elif __linux__
#include "ntohll.cpp"
#else
#pragma message "UNKNOWN COMPILER"
#endif

using namespace std::chrono;

ArcConfig Arc::config;
bool Arc::initializing = true;
struct sockaddr_in Arc::serv_addr;
int64_t Arc::vwap = 0;
std::vector<Trade> Arc::trades;
Quote Arc::quote;

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

    printf("sending order data\n");
    if (vwap < 0) {
        Order order;
        unsigned char bytes_sent = sizeof(order);
        memcpy(buffer, &order, sizeof(order));
        int n = write(socket, buffer, bytes_sent);
        if (n < 0) {
        }

        read_bytes(socket, 32, buffer);
        memcpy(&order, buffer, sizeof(order));
        printf("o> %" PRIu64 " %7s %c $%d x %d\n", order.timestamp, order.symbol, order.side, order.price_c, order.qty);
    }
    return 0;
}

int Arc::calc_vwap() {
    uint64_t period_ns = config.vwap_period_s * 1000000000;

    printf("INITIALIZING\n");

    uint64_t earliest_ns = 0;
    while (initializing) {
        usleep(5000);

        if (earliest_ns == 0) {
            if (trades.size() > 0) {
                auto t = trades.back();
                earliest_ns = t.timestamp;
                printf("0> %" PRIx64 " %" PRIx64 " %7s %8d %4d\n", earliest_ns, t.timestamp, (char *)&t.symbol, t.price_c, t.qty);
            } else {
                continue;
            }
        }

        uint64_t cutoff_ns = earliest_ns + period_ns;
        for (auto t : trades) {
            if (t.timestamp > cutoff_ns) {
                printf("1> %" PRIx64 " %" PRIx64 " %7s %8d %4d\n", earliest_ns, t.timestamp, (char *)&t.symbol, t.price_c, t.qty);
                initializing = false;
            }
        }
    }

    printf("READY\n");

    while (true) {
        uint64_t now_ns = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
        int64_t total_spent = 0;
        int64_t total_contracts = 0;
        uint64_t cutoff_ns = now_ns - (2 * period_ns);
        for (auto t : trades) {
            if (t.timestamp > cutoff_ns) {
                total_spent += t.price_c * t.qty * 100;
                total_contracts += t.qty;
                // printf("v> %" PRIx64 " %" PRIx64 " %7s %8d %4d\n", now_ns, t.timestamp, (char *)&t.symbol, t.price_c, t.qty);
            } else {
                // printf("_> %" PRIx64 " %" PRIx64 " %7s %8d %4d\n", now_ns, t.timestamp, (char *)&t.symbol, t.price_c, t.qty);
            }
        }
        if (total_contracts > 0) {
            vwap = total_spent / total_contracts;
        }
        printf("vwap = %" PRIu64 "\n", vwap);
        usleep(500000);
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
            if (strncmp(quote.symbol, config.symbol, strlen(config.symbol)) == 0 && !initializing) {
                printf("q> %" PRIx64 " %7s $%8d x %3d, $%8d x %3d\n", quote.timestamp, quote.symbol, quote.bid_price_c, quote.bid_qty, quote.ask_price_c, quote.ask_qty);
                if (config.side == 'B' && (quote.ask_price_c * 100 <= vwap)) {
                    printf("BUY: %" PRIi64 "\n", (vwap - quote.ask_price_c * 100));
                } else if (config.side == 'S' && (quote.bid_price_c * 100 >= vwap)) {
                    printf("SELL: %" PRIi64 "\n", (vwap - quote.bid_price_c * 100));
                }
            }
            break;
        }
        case 2: {
            // char buffer[sizeof(Trade)];
            read_bytes(socket, length, buffer);
            char *pos = buffer;
            uint64_t timestamp = ntohll(*(uint64_t*)pos);
            pos += sizeof(uint64_t);
            uint64_t symbol = ntohll(*(uint64_t*)pos);
            pos += sizeof(uint64_t);
            int32_t price_c = ntohl(*(int32_t*)pos);
            pos += sizeof(int32_t);
            uint32_t qty = ntohl(*(int32_t*)pos);
            pos += sizeof(uint32_t);

            if (strncmp((const char *)&symbol, config.symbol, strlen(config.symbol)) == 0) {
                // TODO: custom allocator
                trades.emplace_back(timestamp, symbol, price_c, qty);
            }

            uint64_t now_ns = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
            uint64_t period_ns = config.vwap_period_s * 1000000000;
            uint64_t cutoff_ns = now_ns - period_ns;
            trades.erase(std::remove_if(trades.begin(), trades.end(), [cutoff_ns](Trade &t) { return t.timestamp < cutoff_ns; }), trades.end());
            // printf("t> %" PRIx64 " %7s $%8d x %3d\n", timestamp, (const char *)&symbol, price_c, qty);
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
