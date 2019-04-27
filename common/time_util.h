#include <iomanip>
#include <iostream>

void output_time(uint64_t nnn) {
    constexpr uint64_t second_duration = 1000ULL * 1000ULL * 1000ULL;
    constexpr uint64_t minute_duration = second_duration * 60ULL;
    constexpr uint64_t hour_duration = minute_duration * 60ULL;
    constexpr uint64_t day_duration = hour_duration * 24ULL;

//    using clock = std::chrono::high_resolution_clock;

//    auto now = clock::now();

    //auto days = now.time_since_epoch().count() / day_duration;
    auto days = nnn / day_duration;
    //auto hms = now.time_since_epoch().count() - (days * day_duration);
    auto hms = nnn - (days * day_duration);
    auto hours = hms / hour_duration;
    hms -= hours * hour_duration;
    auto mins = hms / minute_duration;
    hms -= mins * minute_duration;
    auto secs = hms / second_duration;
    auto ns = hms - secs * second_duration;

    std::cout
        << std::setfill('0')
        << std::setw(2) << hours << ":"
        << std::setw(2) << mins << ":"
        << std::setw(2) << secs << "."
        << std::setw(9) << ns;
}

