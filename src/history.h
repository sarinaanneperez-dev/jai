#pragma once
#include "constants.h"
#include "move.h"

namespace jaishi {

// History heuristic + killer moves.
class HistoryTable {
public:
    HistoryTable() { clear(); }
    void clear();

    void update_history(int side, int from, int to, int depth);
    int  get_history(int side, int from, int to) const;

    void update_killer(int ply, Move m);
    Move killer(int ply, int slot) const { return killers_[ply][slot]; }

private:
    int  history_[2][64][64];
    Move killers_[MAX_PLY][2];
};

} // namespace jaishi
