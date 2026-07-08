#pragma once
#include "constants.h"

namespace jaishi {

class Board;

// Full static evaluation from side-to-move perspective, in centipawns.
int evaluate(const Board& b);

// Utility used by search / SEE.
int piece_value_mg(int piece_type);

// Game phase 0..24 (endgame..opening).
int game_phase(const Board& b);

} // namespace jaishi
