#pragma once
#include "constants.h"
#include "move.h"

namespace jaishi {

enum TTFlag : U8 { TT_NONE = 0, TT_EXACT = 1, TT_LOWER = 2, TT_UPPER = 3 };

struct TTEntry {
    U64  key;      // full key
    Move best;
    int  score;    // may be mate-adjusted
    U16  depth;
    U8   flag;
    U8   age;
};

class TranspositionTable {
public:
    TranspositionTable();
    ~TranspositionTable();

    void resize_mb(int mb);
    void clear();
    void new_search() { age_++; }

    // probe: returns true and fills 'out' if key matches.
    bool probe(U64 key, TTEntry& out) const;
    void store(U64 key, Move best, int score, int depth, int ply, TTFlag flag);

private:
    TTEntry* entries_ = nullptr;
    std::size_t count_ = 0;
    U8 age_ = 0;

    static int score_to_tt(int score, int ply);
    static int score_from_tt(int score, int ply);
};

extern TranspositionTable TT;

} // namespace jaishi
