#include "tt.h"
#include <cstdlib>
#include <cstring>

namespace jaishi {

TranspositionTable TT;

TranspositionTable::TranspositionTable() {
    resize_mb(64);
}
TranspositionTable::~TranspositionTable() {
    std::free(entries_);
}

void TranspositionTable::resize_mb(int mb) {
    if (mb < 1) mb = 1;
    std::size_t bytes = static_cast<std::size_t>(mb) * 1024ULL * 1024ULL;
    std::size_t n = bytes / sizeof(TTEntry);
    if (n == 0) n = 1;
    std::free(entries_);
    entries_ = static_cast<TTEntry*>(std::malloc(n * sizeof(TTEntry)));
    count_   = n;
    clear();
}

void TranspositionTable::clear() {
    if (entries_) std::memset(entries_, 0, count_ * sizeof(TTEntry));
    age_ = 0;
}

int TranspositionTable::score_to_tt(int score, int ply) {
    if (score >=  MATE_IN_MAX) return score + ply;
    if (score <= -MATE_IN_MAX) return score - ply;
    return score;
}
int TranspositionTable::score_from_tt(int score, int ply) {
    if (score >=  MATE_IN_MAX) return score - ply;
    if (score <= -MATE_IN_MAX) return score + ply;
    return score;
}

bool TranspositionTable::probe(U64 key, TTEntry& out) const {
    TTEntry& e = entries_[key % count_];
    if (e.key == key && e.flag != TT_NONE) {
        out = e;
        return true;
    }
    return false;
}

void TranspositionTable::store(U64 key, Move best, int score, int depth, int ply, TTFlag flag) {
    TTEntry& e = entries_[key % count_];

    // Always replace empty entries or entries from a different key.
    // For same-key collisions: prefer the entry from the newer search,
    // or the one with greater depth.
    bool replace = false;
    if (e.flag == TT_NONE || e.key != key) {
        replace = true;
    } else if (e.age != age_) {
        replace = true;  // old search generation
    } else if (depth >= e.depth) {
        replace = true;  // same age, greater or equal depth
    }
    // Also replace same-age, same-depth entries (fresh data wins ties).

    if (replace) {
        e.key   = key;
        e.best  = best;
        e.score = score_to_tt(score, ply);
        e.depth = static_cast<U16>(depth);
        e.flag  = static_cast<U8>(flag);
        e.age   = age_;
    }
}

} // namespace jaishi
