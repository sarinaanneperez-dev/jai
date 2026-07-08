#pragma once
#include "constants.h"

namespace jaishi {

class Board;

// Standard perft: counts leaf nodes at exactly `depth` plies.
U64 perft(Board& b, int depth);

// Divide: prints each root move with its perft count. Returns total.
U64 perft_divide(Board& b, int depth);

} // namespace jaishi
