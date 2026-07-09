#pragma once
#include <chrono>
#include <atomic>

namespace jaishi {

struct SearchLimits {
    int  depth = 0;         // 0 = unlimited
    long long movetime_ms = 0; // 0 = unspecified
    long long wtime = 0, btime = 0, winc = 0, binc = 0;
    int  movestogo = 0;
    bool infinite = false;
};

class TimeManager {
public:
    void start(const SearchLimits& limits, int stm);
    bool should_stop() const;       // hard limit — must stop now
    bool should_stop_soft() const;  // soft limit — ok to stop after current depth
    long long elapsed_ms() const;

    long long soft_limit_ms() const { return soft_limit_ms_; }
    long long hard_limit_ms() const { return hard_limit_ms_; }

private:
    std::chrono::steady_clock::time_point start_;
    long long soft_limit_ms_ = 0; // target time per move
    long long hard_limit_ms_ = 0; // absolute stop (emergency)
    bool infinite_ = false;
};

extern std::atomic<bool> STOP_FLAG;

} // namespace jaishi
