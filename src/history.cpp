#include "history.h"
#include <cstring>
#include <cstdlib>


namespace jaishi {

void HistoryTable::clear() {
    std::memset(history_, 0, sizeof(history_));
    std::memset(killers_, 0, sizeof(killers_));
}

void HistoryTable::update_history(int side, int from, int to, int depth) {
    int bonus = depth * depth;
    int& h = history_[side][from][to];
    h += bonus - h * std::abs(bonus) / 16384;
}

int HistoryTable::get_history(int side, int from, int to) const {
    return history_[side][from][to];
}

void HistoryTable::update_killer(int ply, Move m) {
    if (ply < 0 || ply >= MAX_PLY) return;
    if (killers_[ply][0] == m) return;
    killers_[ply][1] = killers_[ply][0];
    killers_[ply][0] = m;
}

} // namespace jaishi
