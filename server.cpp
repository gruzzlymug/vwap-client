#include "server.h"

#include <chrono>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#ifdef __APPLE__
#elif __linux__
#include "ntohll.cpp"
#else
#pragma message "UNKNOWN COMPILER"
#endif

using namespace std::chrono;

Server::Server(int mode)
    : sockfd(-1), mode(mode), done(false) {
    signal(SIGCHLD, SIG_IGN);
}

Server::~Server() {
    close(sockfd);
}

void Server::connect(int port) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        return;
    }

    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        return;
    }
    ::listen(sockfd, 5);
}

void Server::listen() {
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    while (!done) {
        printf("accepting\n");
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            return;
        }

        int pid = fork();
        if (pid < 0) {
            perror("ERROR on fork");
            return;
        }
        if (pid == 0) {
            close(sockfd);
            switch (mode) {
                case 0:
                    send_market_data(newsockfd);
                    break;
                case 1:
                    accept_orders(newsockfd);
                    break;
                default:
                    printf("BAD MODE %d\n", mode);
            }
            exit(0);
        } else {
            close(newsockfd);
        }
    }
}

void Server::stop() {
    done = true;
}

void Server::send_market_data(int socket) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> delay_range(1, 100);
    std::uniform_int_distribution<> data_type(0, 2);

    while (true) {
        switch (data_type(gen)) {
            case 0:
            case 1:
                send_trade(socket);
                break;
            case 2:
                send_quote(socket);
                break;
            default:
                break;
        }
        unsigned int delay_us = delay_range(gen) * 1000;
        usleep(delay_us);
    }
}

int Server::read_bytes(int socket, unsigned int num_to_read, char *buffer) {
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

void Server::send_quote(int socket) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> price_range(-25, 25);
    std::uniform_int_distribution<> contract_range(1, 13);
    std::uniform_int_distribution<> side_range(0, 1);

    uint64_t now_ns = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();

    uint64_t now_s = now_ns / 1000000000;
    uint16_t offset = now_s % 6283;
    int32_t trend = sin(offset / 3141.5f) * 250;
    int32_t price_c = 991330 + trend + price_range(gen) * 2;

    char range = price_range(gen);
    char spread = 10;

    Quote quote;
    quote.timestamp = now_ns;
    quote.symbol = 0;
    strncpy((char *)&quote.symbol, "BTC.USD", strlen("BTC.USD"));
    quote.bid_price_c = price_c + range - spread;
    quote.bid_qty = contract_range(gen);
    quote.ask_price_c = price_c + range + spread;
    quote.ask_qty = contract_range(gen);

    char buffer[256];
    unsigned char quote_size = sizeof(quote);

    // printf("Sending Quote\n");
    buffer[0] = quote_size;
    buffer[1] = 0x01;
    int n = write(socket, buffer, 2);
    if (n < 0) {
    }

    char *pos = buffer;
    *(uint64_t*)pos = htonll(quote.timestamp);
    pos += sizeof(uint64_t);
    *(uint64_t*)pos = htonll(quote.symbol);
    pos += sizeof(uint64_t);
    *(int32_t*)pos = htonl(quote.bid_price_c);
    pos += sizeof(int32_t);
    *(uint32_t*)pos = htonl(quote.bid_qty);
    pos += sizeof(uint32_t);
    *(int32_t*)pos = htonl(quote.ask_price_c);
    pos += sizeof(int32_t);
    *(uint32_t*)pos = htonl(quote.ask_qty);
    pos += sizeof(uint32_t);

    int p = write(socket, buffer, quote_size);
    if (p < 0) {
    }
}

void Server::send_trade(int socket) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> price_range(-10, 10);
    std::uniform_int_distribution<> contract_range(1, 13);
    std::uniform_int_distribution<> symbol_range(0, 2);

    uint64_t now_ns = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
    Trade trade;
    trade.timestamp = now_ns;
    switch (symbol_range(gen)) {
        case 0: {
            uint64_t now_s = now_ns / 1000000000;
            uint16_t offset = now_s % 6283;
            int32_t trend = sin(offset / 3141.5f) * 250;

            strncpy((char *)&trade.symbol, "BTC.USD", strlen("BTC.USD")) ;
            trade.price_c = 991330 + trend + price_range(gen) * 2;
            break;
        }
        case 1:
            strncpy((char *)&trade.symbol, "ETH.USD", strlen("ETH.USD")) ;
            trade.price_c = 82050 + price_range(gen);
            break;
        default:
            strncpy((char *)&trade.symbol, "XYZ.USD", strlen("XYZ.USD")) ;
            trade.price_c = 1000 + price_range(gen);
    }
    trade.qty = contract_range(gen);

    char header[2];
    unsigned char bytes_sent = sizeof(trade);

    // printf("Sending Trade\n");
    header[0] = bytes_sent;
    header[1] = 0x02;
    int n = write(socket, header, 2);
    if (n < 0) {
    }

    // TODO: clean up buffer use
    char buffer[sizeof(trade)];
    char *pos = buffer;
    *(uint64_t*)pos = htonll(trade.timestamp);
    pos += sizeof(uint64_t);
    *(uint64_t*)pos = htonll(trade.symbol);
    pos += sizeof(uint64_t);
    *(int32_t*)pos = htonl(trade.price_c);
    pos += sizeof(int32_t);
    *(uint32_t*)pos = htonl(trade.qty);
    pos += sizeof(uint32_t);

    // send(0, buffer, (int)(pos - buffer), 0);
    int p = write(socket, buffer, (int)(pos - buffer));
    if (p < 0) {
    }
}

void Server::accept_orders(int socket) {
    char buffer[256];
    Order order;

    printf("Accepting Orders\n");
    unsigned char bytes_needed = 25; //sizeof(order);

    while (true) {
        int n = read_bytes(socket, bytes_needed, buffer);
        if (n == 0) {
            char *pos = buffer;

            order.timestamp = ntohll(*(uint64_t*)pos);
            pos += sizeof(uint64_t);
            order.symbol = ntohll(*(uint64_t*)pos);
            pos += sizeof(uint64_t);
            order.side = *(uint8_t*)pos;
            pos += sizeof(uint8_t);
            order.price_c = ntohl(*(int32_t*)pos);
            pos += sizeof(int32_t);
            order.qty = ntohl(*(int32_t*)pos);
            pos += sizeof(uint32_t);

            printf("o> %" PRIx64 " %7s %c $%8d x %4d\n", order.timestamp, (char *)&order.symbol, order.side, order.price_c, order.qty);
        }
    }
}
