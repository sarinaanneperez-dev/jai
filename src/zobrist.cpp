#include "zobrist.h"

namespace jaishi {

ZobristKeys ZOBRIST;

namespace {
struct Xoshiro {
    U64 s;
    U64 next() {
        // splitmix64 — plenty for zobrist init
        U64 z = (s += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
};
}

void zobrist_init() {
    Xoshiro rng{ 0xC0FFEE1234567890ULL };
    for (int p = 0; p < 12; ++p)
        for (int sq = 0; sq < 64; ++sq)
            ZOBRIST.piece[p][sq] = rng.next();
    for (int i = 0; i < 16; ++i) ZOBRIST.castling[i] = rng.next();
    for (int f = 0; f < 8; ++f)  ZOBRIST.en_passant[f] = rng.next();
    ZOBRIST.side_to_move = rng.next();
}

} // namespace jaishi
