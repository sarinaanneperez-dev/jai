#include "time_manager.h"
#include "constants.h"

namespace jaishi {

std::atomic<bool> STOP_FLAG{ false };

void TimeManager::start(const SearchLimits& limits, int stm) {
    start_ = std::chrono::steady_clock::now();
    infinite_ = limits.infinite;
    hard_limit_ms_ = 0;

    if (limits.movetime_ms > 0) {
        hard_limit_ms_ = limits.movetime_ms;
        return;
    }
    long long remaining = (stm == WHITE) ? limits.wtime : limits.btime;
    long long inc       = (stm == WHITE) ? limits.winc  : limits.binc;
    if (remaining > 0) {
        int mtg = limits.movestogo > 0 ? limits.movestogo : 30;
        hard_limit_ms_ = remaining / mtg + inc / 2;
        if (hard_limit_ms_ > remaining - 50) hard_limit_ms_ = remaining - 50;
        if (hard_limit_ms_ < 10) hard_limit_ms_ = 10;
    }
}

long long TimeManager::elapsed_ms() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_).count();
}

bool TimeManager::should_stop() const {
    if (STOP_FLAG.load(std::memory_order_relaxed)) return true;
    if (infinite_) return false;
    if (hard_limit_ms_ > 0 && elapsed_ms() >= hard_limit_ms_) return true;
    return false;
}

} // namespace jaishi
