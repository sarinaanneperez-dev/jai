#pragma once
#include "constants.h"

namespace jaishi {

// Zobrist keys. Initialized once via zobrist_init().
struct ZobristKeys {
    U64 piece[12][64];
    U64 castling[16];
    U64 en_passant[8]; // by file
    U64 side_to_move;
};

extern ZobristKeys ZOBRIST;

void zobrist_init();

} // namespace jaishi
