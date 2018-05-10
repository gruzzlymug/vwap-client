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

using namespace std;
using namespace std::chrono;

ArcConfig Arc::config_;
atomic<Arc::State> Arc::state_ {Arc::State::INIT};
struct sockaddr_in Arc::serv_addr_;
int64_t Arc::vwap_ = 0;
vector<Trade> Arc::trades_;
vector<Order> Arc::orders_;

Arc::Arc() {
}

Arc::~Arc() {
}

int Arc::start(ArcConfig *new_config) {
    //printf("%s:%d\n", config.market_server_ip, config.market_server_port);
    //printf("%s:%d\n", config.order_server_ip, config.order_server_port);
    memcpy(&config_, new_config, sizeof(ArcConfig));

    int market_socket = connect(config_.market_server_ip, config_.market_server_port);
    int order_socket = connect(config_.order_server_ip, config_.order_server_port);
    thread t1(stream_market_data, market_socket);
    thread t2(send_order_data, order_socket);
    thread t3(calc_vwap);
    thread t4(handle_kb);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    close(order_socket);
    close(market_socket);
    return 0;
}

int Arc::stream_market_data(int socket) {
    char buffer[256];
    bzero(buffer, 256);
    uint64_t last_order_ns = 0;
    uint64_t order_timeout_ns = config_.order_timeout_s * 1000000000;

    while (state_ != Arc::State::QUIT) {
        read_bytes(socket, 2, buffer);
        unsigned int length = (unsigned char) buffer[0];
        unsigned int message_type = (unsigned char) buffer[1];
        //printf("-> %d %d\n", length, message_type);
        switch (message_type) {
        case 1: {
            Quote quote;
            read_bytes(socket, length, buffer);
            char *pos = buffer;
            quote.timestamp = ntohll(*(uint64_t*)pos);
            pos += sizeof(uint64_t);
            quote.symbol = ntohll(*(uint64_t*)pos);
            pos += sizeof(uint64_t);
            quote.bid_price_c = ntohl(*(int32_t*)pos);
            pos += sizeof(int32_t);
            quote.bid_qty = ntohl(*(uint32_t*)pos);
            pos += sizeof(uint32_t);
            quote.ask_price_c = ntohl(*(int32_t*)pos);
            pos += sizeof(int32_t);
            quote.ask_qty = ntohl(*(uint32_t*)pos);
            pos += sizeof(uint32_t);

            uint64_t now_ns = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
            bool order_timeout_expired = now_ns > (last_order_ns + order_timeout_ns);
            bool can_order = state_ == Arc::State::RUN && order_timeout_expired;
            if (!order_timeout_expired) {
                //printf(".....\n");
            }
            if (can_order && strncmp((char *)&quote.symbol, config_.symbol, strlen(config_.symbol)) == 0) {
                //printf("q> %" PRIx64 " %7s $%8d x %3d, $%8d x %3d\n", quote.timestamp, (char *)&quote.symbol, quote.bid_price_c, quote.bid_qty, quote.ask_price_c, quote.ask_qty);
                char action = 'X';
                if (config_.side == 'B' && (quote.ask_price_c * 100 <= vwap_)) {
                    //printf("BUY: %" PRIi64 "\n", (vwap_ - quote.ask_price_c * 100));
                    action = config_.side;
                } else if (config_.side == 'S' && (quote.bid_price_c * 100 >= vwap_)) {
                    //printf("SELL: %" PRIi64 "\n", (vwap_ - quote.bid_price_c * 100));
                    action = config_.side;
                }
                if (action != 'X') {
                    Order order;
                    order.timestamp = quote.timestamp;
                    order.symbol = quote.symbol;
                    order.side = action;
                    if (action == 'B') {
                        order.price_c = quote.ask_price_c;
                        order.qty = min(config_.qty_max, quote.ask_qty);
                    } else {
                        order.price_c = quote.bid_price_c;
                        order.qty = min(config_.qty_max, quote.bid_qty);
                    }
                    orders_.push_back(order);
                    last_order_ns = now_ns;
                }
            }
            break;
        }
        case 2: {
            read_bytes(socket, length, buffer);
            char *pos = buffer;
            uint64_t timestamp = ntohll(*(uint64_t*)pos);
            pos += sizeof(uint64_t);
            uint64_t symbol = ntohll(*(uint64_t*)pos);
            pos += sizeof(uint64_t);
            int32_t price_c = ntohl(*(int32_t*)pos);
            pos += sizeof(int32_t);
            uint32_t qty = ntohl(*(uint32_t*)pos);
            pos += sizeof(uint32_t);

            if (strncmp((const char *)&symbol, config_.symbol, strlen(config_.symbol)) == 0) {
                // TODO: custom allocator / array
                trades_.emplace_back(timestamp, symbol, price_c, qty);
            }

            // printf("t> %" PRIx64 " %7s $%8d x %3d\n", timestamp, (const char *)&symbol, price_c, qty);
            break;
        }
        default:
            printf("PROBLEM\n");
        }
    }
    return 0;
}

int Arc::calc_vwap() {
    uint64_t period_ns = config_.vwap_period_s * 1000000000;
    uint64_t earliest_ns = 0;

    printf("INITIALIZING\n");
    while (state_ == Arc::State::INIT) {
        usleep(500);
        if (earliest_ns == 0) {
            if (trades_.size() > 0) {
                auto t = trades_.front();
                earliest_ns = t.timestamp;
                printf("0> %" PRIx64 " %" PRIx64 " %7s %8d %4d\n", earliest_ns, t.timestamp, (char *)&t.symbol, t.price_c, t.qty);
            } else {
                continue;
            }
        }

        uint64_t cutoff_ns = earliest_ns + period_ns;
        auto t = trades_.back();
        uint64_t newest_ns = t.timestamp;
        if (newest_ns > cutoff_ns) {
            int64_t total_spent = 0;
            int64_t total_contracts = 0;
            for (auto it_trade = trades_.rbegin(); it_trade != trades_.rend(); ++it_trade) {
                total_spent += it_trade->price_c * it_trade->qty * 100;
                total_contracts += it_trade->qty;
            }
            vwap_ = total_spent / total_contracts;
            printf("1> %" PRIx64 " %" PRIx64 " %7s %8d %4d\n", earliest_ns, t.timestamp, (char *)&t.symbol, t.price_c, t.qty);
            state_ = Arc::State::RUN;
        }
    }

    printf("READY\n");
    while (state_ != Arc::State::QUIT) {
        uint64_t now_ns = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
        int64_t total_spent = 0;
        int64_t total_contracts = 0;
        uint64_t cutoff_ns = now_ns - period_ns;
        for (auto t : trades_) {
            if (t.timestamp > cutoff_ns) {
                total_spent += t.price_c * t.qty * 100;
                total_contracts += t.qty;
                // printf("v> %" PRIx64 " %" PRIx64 " %7s %8d %4d\n", now_ns, t.timestamp, (char *)&t.symbol, t.price_c, t.qty);
            } else {
                // printf("_> %" PRIx64 " %" PRIx64 " %7s %8d %4d\n", now_ns, t.timestamp, (char *)&t.symbol, t.price_c, t.qty);
            }
        }
        if (total_contracts > 0) {
            vwap_ = total_spent / total_contracts;
        }
        printf("vwap = %" PRIu64 "\n", vwap_);

        trades_.erase(remove_if(trades_.begin(), trades_.end(), [cutoff_ns](Trade &t) { return t.timestamp < cutoff_ns; }), trades_.end());

        usleep(500000);
    }
    return 0;
}

int Arc::send_order_data(int socket) {
    char buffer[25];
    bzero(buffer, 25);

    uint64_t cutoff_ns = 0;
    while (state_ != Arc::State::QUIT) {
        for (auto it_order = orders_.rbegin(); it_order != orders_.rend(); ++it_order) {
            if (it_order->timestamp <= cutoff_ns) {
                continue;
            }

            //printf(">> %" PRIx64 " %7s %c $%d x %d\n", it_order->timestamp, (char *)&it_order->symbol, it_order->side, it_order->price_c, it_order->qty);

            char buffer[25];
            char *pos = buffer;
            *(uint64_t*)pos = htonll(it_order->timestamp);
            pos += sizeof(uint64_t);
            *(uint64_t*)pos = htonll(it_order->symbol);
            pos += sizeof(uint64_t);
            *(uint8_t*)pos = it_order->side;
            pos += sizeof(uint8_t);
            *(int32_t*)pos = htonl(it_order->price_c);
            pos += sizeof(int32_t);
            *(uint32_t*)pos = htonl(it_order->qty);
            pos += sizeof(uint32_t);

            // NOTE this will be 25
            int bytes_buffered = (int)(pos - buffer);
            int p = write(socket, buffer, bytes_buffered);
            if (p > 0) {
                cutoff_ns = it_order->timestamp;
            } else {
                printf("ERROR could not send order\n");
            }
        }

        orders_.erase(remove_if(orders_.begin(), orders_.end(), [cutoff_ns](Order &o) { return o.timestamp <= cutoff_ns; }), orders_.end());

        usleep(50000);
    }
    return 0;
}

int Arc::handle_kb() {
    while (state_ != Arc::State::QUIT) {
        printf("start\n");
        char c = getchar();
        if (c == 'q' || c == 'Q') {
            printf("QUITTING\n");
            state_ = Arc::State::QUIT;
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
    struct hostent *server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "ERROR no such host\n");
        return -1;
    }

    bzero((char *) &serv_addr_, sizeof(serv_addr_));
    serv_addr_.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr_.sin_addr.s_addr, server->h_length);
    serv_addr_.sin_port = htons(port);
    if (::connect(sockfd, (struct sockaddr *) &serv_addr_, sizeof(serv_addr_)) < 0) {
        perror("ERROR connecting");
        return -1;
    }
    return sockfd;
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
