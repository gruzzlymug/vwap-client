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
#pragma message "UNKNOWN PLATFORM"
#endif

using namespace std;
using namespace std::chrono;

ArcConfig Arc::config_;
atomic<Arc::State> Arc::state_ {Arc::State::INIT};
struct sockaddr_in Arc::serv_addr_;
int64_t Arc::vwap_ = 0;

const int Arc::max_trades;
Trade Arc::all_trades[2 * max_trades];
uint16_t Arc::t_first = 0;
uint16_t Arc::t_next = 0;

const int Arc::max_orders;
Order Arc::all_orders[2 * max_orders];
uint16_t Arc::o_first = 0;
uint16_t Arc::o_next = 0;

uint64_t Arc::last_order_ns = 0;

uint64_t last_send_ns = 0;

Arc::Arc() {
}

Arc::~Arc() {
}

int Arc::start(ArcConfig *config) {
    // printf("%llx:%llx\n",
    //     (unsigned long long)all_trades,
    //     (unsigned long long)(all_trades + (2 * max_trades - 1)));
    //printf("%s:%d\n", config.market_server_ip, config.market_server_port);
    //printf("%s:%d\n", config.order_server_ip, config.order_server_port);
    memcpy(&config_, config, sizeof(ArcConfig));

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

    while (state_ != Arc::State::QUIT) {
        read_bytes(socket, 2, buffer);
        unsigned int length = (unsigned char) buffer[0];
        unsigned int message_type = (unsigned char) buffer[1];
        read_bytes(socket, length, buffer);
        //printf("-> %d %d\n", length, message_type);
        switch (message_type) {
        case 1: {
            stream_quote(buffer, length);
            break;
        }
        case 2: {
            stream_trade(buffer, length);
            break;
        }
        default:
            printf("PROBLEM\n");
        }
    }
    return 0;
}

int Arc::stream_quote(char *buffer, size_t length) {
    Quote quote;
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
    uint64_t order_timeout_ns = config_.order_timeout_s * 1000000000;
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
            for (uint16_t idx_order = o_first; idx_order < o_next; ++idx_order) {
                Order *op = (Order *)(all_orders + idx_order);
                if (op->timestamp <= last_send_ns) {
                    ++o_first;
                }
            }

            if (o_first > max_orders) {
                // TODO bug if o_next wraps before copy
                // TODO bug if moved while being read
                memcpy(all_orders, all_orders + o_first, (o_next - o_first) * sizeof(Order));
                o_next -= o_first;
                o_first = 0;
            }

            void *vp = all_orders + o_next;
            Order *op = new(vp) Order;
            op->timestamp = quote.timestamp;
            op->symbol = quote.symbol;
            op->side = action;
            if (action == 'B') {
                op->price_c = quote.ask_price_c;
                op->qty = min(config_.qty_max, quote.ask_qty);
            } else {
                op->price_c = quote.bid_price_c;
                op->qty = min(config_.qty_max, quote.bid_qty);
            }
            last_order_ns = now_ns;

            ++o_next;
            o_next %= (max_orders * 2);
        }
    }
    return 0;
}

int Arc::stream_trade(char *buffer, size_t length) {
    char *pos = buffer;
    uint64_t timestamp = ntohll(*(uint64_t*)pos);
    pos += sizeof(uint64_t);
    uint64_t symbol = ntohll(*(uint64_t*)pos);
    pos += sizeof(uint64_t);
    int32_t price_c = ntohl(*(int32_t*)pos);
    pos += sizeof(int32_t);
    uint32_t qty = ntohl(*(uint32_t*)pos);
    pos += sizeof(uint32_t);

    // TODO precalculate period_ns and remove duplication
    uint64_t now_ns = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
    uint64_t period_ns = config_.vwap_period_s * 1000000000;
    uint64_t cutoff_ns = now_ns - period_ns;
    for (uint16_t idx_trade = t_first; idx_trade < t_next; ++idx_trade) {
        Trade *tp = (Trade *)(all_trades + idx_trade);
        if (tp->timestamp < cutoff_ns) {
            ++t_first;
        }
    }

    if (t_first > max_trades) {
        // TODO bug if t_next wraps before copy
        memcpy(all_trades, all_trades + t_first, (t_next - t_first) * sizeof(Trade));
        t_next -= t_first;
        t_first = 0;
    }

    if (strncmp((const char *)&symbol, config_.symbol, strlen(config_.symbol)) == 0) {
        void *vp = all_trades + t_next;
        // printf("%4d %llx\n", (int) t_next, (unsigned long long) vp);
        Trade *tp = new(vp) Trade;
        tp->timestamp = timestamp;
        tp->symbol = symbol;
        tp->price_c = price_c;
        tp->qty = qty;
        // printf("t> %" PRIx64 " %7s $%8d x %3d\n", tp->timestamp, (const char *)&tp->symbol, tp->price_c, tp->qty);

        ++t_next;
        t_next %= (max_trades * 2);
    }
    return 0;
}

int Arc::calc_vwap() {
    // TODO precalculate
    uint64_t period_ns = config_.vwap_period_s * 1000000000;
    uint64_t earliest_ns = 0;

    printf("INITIALIZING\n");
    while (state_ == Arc::State::INIT) {
        usleep(500);
        if (earliest_ns == 0) {
            if (t_next > t_first) {
                Trade *tp = &all_trades[0];
                earliest_ns = tp->timestamp;
                printf("0> %" PRIx64 " %" PRIx64 " %7s %8d %4d\n", earliest_ns, tp->timestamp, (char *)&tp->symbol, tp->price_c, tp->qty);
            } else {
                continue;
            }
        }

        uint64_t cutoff_ns = earliest_ns + period_ns;
        Trade *tp = &all_trades[t_next - 1];
        uint64_t newest_ns = tp->timestamp;
        if (newest_ns > cutoff_ns) {
            int64_t total_spent = 0;
            int64_t total_contracts = 0;
            for (uint16_t idx_trade = t_next - 1; idx_trade != t_first; --idx_trade) {
                total_spent += all_trades[idx_trade].price_c * all_trades[idx_trade].qty * 100;
                total_contracts += all_trades[idx_trade].qty;
            }
            vwap_ = total_spent / total_contracts;
            printf("1> %" PRIx64 " %" PRIx64 " %7s %8d %4d\n", earliest_ns, tp->timestamp, (char *)&tp->symbol, tp->price_c, tp->qty);
            state_ = Arc::State::RUN;
        }
    }

    printf("READY\n");
    while (state_ != Arc::State::QUIT) {
        uint64_t now_ns = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
        int64_t total_spent = 0;
        int64_t total_contracts = 0;
        uint64_t cutoff_ns = now_ns - period_ns;
        for (uint16_t idx_trade = t_first; idx_trade < t_next; ++idx_trade) {
            Trade *tp = (Trade *)(all_trades + idx_trade);
            if (tp->timestamp > cutoff_ns) {
                total_spent += tp->price_c * tp->qty * 100;
                total_contracts += tp->qty;
            }
            // printf("v> %4d %" PRIx64 " %" PRIx64 " %s %8d %4d\n", t_first, now_ns, tp->timestamp, (char *)&tp->symbol, tp->price_c, tp->qty);
        }
        if (total_contracts > 0) {
            vwap_ = total_spent / total_contracts;
        }
        printf("vwap = %" PRIu64 "\n", vwap_);

        usleep(500000);
    }
    return 0;
}

int Arc::send_order_data(int socket) {
    char buffer[25];
    bzero(buffer, 25);

    while (state_ != Arc::State::QUIT) {
        for (uint16_t idx_order = o_first; idx_order < o_next; ++idx_order) {
            Order *op = (Order *)(all_orders + idx_order);
            if (op->timestamp <= last_send_ns) {
                continue;
            }

            printf(">> %" PRIx64 " %7s %c $%d x %d\n", op->timestamp, (char *)&op->symbol, op->side, op->price_c, op->qty);

            char buffer[25];
            char *pos = buffer;
            *(uint64_t*)pos = htonll(op->timestamp);
            pos += sizeof(uint64_t);
            *(uint64_t*)pos = htonll(op->symbol);
            pos += sizeof(uint64_t);
            *(uint8_t*)pos = op->side;
            pos += sizeof(uint8_t);
            *(int32_t*)pos = htonl(op->price_c);
            pos += sizeof(int32_t);
            *(uint32_t*)pos = htonl(op->qty);
            pos += sizeof(uint32_t);

            // NOTE this will be 25
            int bytes_buffered = (int)(pos - buffer);
            int p = write(socket, buffer, bytes_buffered);
            if (p > 0) {
                last_send_ns = op->timestamp;
            } else {
                printf("ERROR could not send order\n");
            }
        }

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
