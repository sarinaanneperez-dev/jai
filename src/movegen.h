#pragma once
#include "constants.h"
#include "move.h"

namespace jaishi {

class Board;

void movegen_init();

extern U64 KNIGHT_ATTACKS[64];
extern U64 KING_ATTACKS[64];
extern U64 PAWN_ATTACKS[2][64];

U64 bishop_attacks(int sq, U64 occ);
U64 rook_attacks(int sq, U64 occ);
inline U64 queen_attacks(int sq, U64 occ) { return bishop_attacks(sq, occ) | rook_attacks(sq, occ); }

// Portable bit helpers
#ifdef _MSC_VER
    #include <intrin.h>
    #pragma intrinsic(_BitScanForward64, _popcnt64)
    inline int popcount(U64 x) { return static_cast<int>(_popcnt64(x)); }
    inline int lsb(U64 x) {
        unsigned long idx;
        _BitScanForward64(&idx, x);
        return static_cast<int>(idx);
    }
#else
    inline int popcount(U64 x) { return __builtin_popcountll(x); }
    inline int lsb(U64 x)     { return __builtin_ctzll(x); }
#endif

inline int pop_lsb(U64& x) {
    int s = lsb(x);
    x &= x - 1;
    return s;
}

void generate_all_moves(Board& b, MoveList& list);
void generate_captures(Board& b, MoveList& list);

} // namespace jaishi
