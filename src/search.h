#pragma once
#include "constants.h"
#include "move.h"
#include "history.h"
#include "time_manager.h"
#include <atomic>
#include <string>

namespace jaishi {

class Board;

struct SearchStats {
    long long nodes = 0;
    int  depth_reached = 0;
    long long time_ms = 0;
    int  score = 0;
    Move best_move = NULL_MOVE;
};

// Iterative-deepening search. Prints UCI info lines. Returns best move found.
Move search_best_move(Board& board, const SearchLimits& limits);

// SEE helper (also usable by other code).
int see_capture(Board& b, Move m);

extern SearchStats LAST_SEARCH;

} // namespace jaishi
