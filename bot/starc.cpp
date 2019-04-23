#include "starc.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>

#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

using namespace std;
using namespace std::chrono;

Starc::Starc() : state_(State::INIT), t_first_(0), t_next_(0) {
    memset(&quote_, 0, sizeof(quote_));
}

int Starc::run(ArcConfig *config) {
    memcpy(&config_, config, sizeof(*config));
    int market_socket = connect(config_.market_server_ip, config_.market_server_port);
    int order_socket = connect(config_.order_server_ip, config_.order_server_port);

    while (state_ != State::QUIT) {
        stream_market_data(market_socket, order_socket);
    }

    close(market_socket);
    close(order_socket);

    return 0;
}

int Starc::connect(char *hostname, int port) {
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

int Starc::stream_market_data(int market_socket, int order_socket) {
    read_bytes(market_socket, 2, buffer_);
    unsigned int length = (unsigned char) buffer_[0];
    unsigned int message_type = (unsigned char) buffer_[1];
    read_bytes(market_socket, length, buffer_);
    switch (message_type) {
    case 1: {
        bool quoteSet = deserialize_quote(buffer_, length);
        if (quoteSet && ready_to_order()) {
            place_order(order_socket);
        }
        break;
    }
    case 2: {
        bool tradeAdded = deserialize_trade(buffer_, length);
        if (tradeAdded) {
            trim_trades();
            calc_vwap();
        }
        break;
    }
    default:
        // TODO attempt to recover, limit retries
        perror("ERROR unexpected message type");
        return -1;
    }
    return 0;
}

// TODO deal with byte ordering
bool Starc::deserialize_quote(char *buffer, size_t length) {
    char *pos = buffer;
    quote_.timestamp = (*(uint64_t*)pos);
    pos += sizeof(uint64_t);
    quote_.symbol = (*(uint64_t*)pos);
    pos += sizeof(uint64_t);
    quote_.bid_price_c = ntohl(*(int32_t*)pos);
    pos += sizeof(int32_t);
    quote_.bid_qty = ntohl(*(uint32_t*)pos);
    pos += sizeof(uint32_t);
    quote_.ask_price_c = ntohl(*(int32_t*)pos);
    pos += sizeof(int32_t);
    quote_.ask_qty = ntohl(*(uint32_t*)pos);
    pos += sizeof(uint32_t);

    if (quote_.symbol == *reinterpret_cast<uint64_t *>(config_.symbol)) {
        return true;
    }
    return false;
}

bool Starc::ready_to_order() {
    uint64_t now_ns = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
    uint64_t order_timeout_ns = config_.order_timeout_s * 1000000000;
    bool order_timeout_expired = now_ns > (last_order_ns_ + order_timeout_ns);
    bool can_order = state_ == Starc::State::RUN && order_timeout_expired;

    bool price_within_range = false;
    if (config_.side == 'B' && (quote_.ask_price_c * 100 <= vwap_)) {
        price_within_range = true;
    } else if (config_.side == 'S' && (quote_.bid_price_c * 100 >= vwap_)) {
        price_within_range = true;
    }

    return can_order && price_within_range;
}

int Starc::place_order(int socket) {
    // printf("q> %" PRIx64 " %7s $%8d x %3d, $%8d x %3d\n", quote_.timestamp, (char *)&quote_.symbol, quote_.bid_price_c, quote_.bid_qty, quote_.ask_price_c, quote_.ask_qty);
    Order order;
    order.timestamp = quote_.timestamp;
    order.symbol = quote_.symbol;
    order.side = config_.side;
    if (config_.side == 'B') {
        order.price_c = quote_.ask_price_c;
        order.qty = min(config_.qty_max, quote_.ask_qty);
    } else {
        order.price_c = quote_.bid_price_c;
        order.qty = min(config_.qty_max, quote_.bid_qty);
    }
    serialize_order(socket, order);
    return 0;
}

void Starc::serialize_order(int socket, const Order& order) {
    // printf(">> %" PRIx64 " %7s %c $%d x %d\n", order.timestamp, (char *)&order.symbol, order.side, order.price_c, order.qty);
    char *pos = buffer_;
    *(uint64_t*)pos = (order.timestamp);
    pos += sizeof(uint64_t);
    *(uint64_t*)pos = (order.symbol);
    pos += sizeof(uint64_t);
    *(uint8_t*)pos = order.side;
    pos += sizeof(uint8_t);
    *(int32_t*)pos = htonl(order.price_c);
    pos += sizeof(int32_t);
    *(uint32_t*)pos = htonl(order.qty);
    pos += sizeof(uint32_t);

    int bytes_buffered = (int)(pos - buffer_);
    int p = write(socket, buffer_, bytes_buffered);
    if (p > 0) {
        last_order_ns_ = order.timestamp;
    } else {
        printf("ERROR could not send order\n");
    }
}

bool Starc::deserialize_trade(char *buffer, size_t length) {
    char *pos = buffer;
    uint64_t timestamp = (*(uint64_t*)pos);
    pos += sizeof(uint64_t);
    uint64_t symbol = (*(uint64_t*)pos);
    pos += sizeof(uint64_t);
    int32_t price_c = ntohl(*(int32_t*)pos);
    pos += sizeof(int32_t);
    uint32_t qty = ntohl(*(uint32_t*)pos);
    pos += sizeof(uint32_t);

    if (symbol == *reinterpret_cast<uint64_t *>(config_.symbol)) {
        void *vp = all_trades_ + t_next_;
        Trade *tp = new(vp) Trade;
        tp->timestamp = timestamp;
        tp->symbol = symbol;
        tp->price_c = price_c;
        tp->qty = qty;
        printf("t> %" PRIx64 " %7s $%8d x %3d\n", tp->timestamp, (const char *)&tp->symbol, tp->price_c, tp->qty);
        ++t_next_;
        return true;
    }
    return false;
}

void Starc::trim_trades() {
    // TODO precalculate period_ns and remove duplication
    uint64_t now_ns = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
    uint64_t period_ns = config_.vwap_period_s * 1000000000;
    uint64_t cutoff_ns = now_ns - period_ns;
    for (uint16_t idx_trade = t_next_ - 1; idx_trade > t_first_; --idx_trade) {
        Trade *tp = (Trade *)(all_trades_ + idx_trade);
        if (tp->timestamp < cutoff_ns) {
            if (state_ == State::INIT) {
                state_ = State::RUN;
            }
            t_first_ = idx_trade;
            break;
        }
    }

    if (t_first_ > MAX_TRADES || t_next_ > (MAX_TRADES * 2)) {
        if (t_first_ == 0) {
            perror("ERROR trade array full");
            exit(1);
        }
        memmove(all_trades_, all_trades_ + t_first_, (t_next_ - t_first_) * sizeof(Trade));
        t_next_ -= t_first_;
        t_first_ = 0;
    }
}

void Starc::calc_vwap() {
    int64_t total_spent = 0;
    int64_t total_contracts = 0;
    for (uint16_t idx_trade = t_first_; idx_trade < t_next_; ++idx_trade) {
        Trade *tp = (Trade *)(all_trades_ + idx_trade);
        total_spent += tp->price_c * tp->qty * 100;
        total_contracts += tp->qty;
    }
    if (total_contracts > 0) {
        vwap_ = total_spent / total_contracts;
    }
    printf("vwap = %" PRIu64 "\n", vwap_);
}

int Starc::read_bytes(int socket, unsigned int num_to_read, char *buffer) {
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
