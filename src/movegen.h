#pragma once
#include "constants.h"
#include "move.h"

namespace jaishi {

class Board;

void movegen_init();

// Precomputed leaper attacks (initialized by movegen_init()).
extern U64 KNIGHT_ATTACKS[64];
extern U64 KING_ATTACKS[64];
extern U64 PAWN_ATTACKS[2][64]; // by color, from square

// Classical ray-scan slider attacks (correct, no magics).
U64 bishop_attacks(int sq, U64 occ);
U64 rook_attacks(int sq, U64 occ);
inline U64 queen_attacks(int sq, U64 occ) { return bishop_attacks(sq, occ) | rook_attacks(sq, occ); }

// Bit helpers
inline int  popcount(U64 x) { return __builtin_popcountll(x); }
inline int  lsb(U64 x) { return __builtin_ctzll(x); }
inline int  pop_lsb(U64& x) { int s = lsb(x); x &= x - 1; return s; }

// Move generation. Both produce pseudo-legal moves; legality is checked in
// Board::make_move by verifying own king is not attacked.
void generate_all_moves(Board& b, MoveList& list);
void generate_captures(Board& b, MoveList& list); // captures + promotions

} // namespace jaishi
