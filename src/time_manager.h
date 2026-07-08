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
    bool should_stop() const;
    long long elapsed_ms() const;

private:
    std::chrono::steady_clock::time_point start_;
    long long hard_limit_ms_ = 0; // absolute stop
    bool infinite_ = false;
};

extern std::atomic<bool> STOP_FLAG;

} // namespace jaishi
