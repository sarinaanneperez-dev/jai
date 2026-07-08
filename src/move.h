#pragma once
#include "constants.h"
#include <string>

namespace jaishi {

// Move encoding (16 bits):
//  bits  0- 5 : from square
//  bits  6-11 : to square
//  bits 12-13 : promotion piece type (0=N, 1=B, 2=R, 3=Q) [only if promo flag]
//  bits 14-15 : flags: 0=normal, 1=promotion, 2=en-passant, 3=castling
using Move = std::uint16_t;
constexpr Move NULL_MOVE = 0;

constexpr Move make_move(int from, int to, int flag = 0, int promo = 0) {
    return static_cast<Move>((from & 0x3F) | ((to & 0x3F) << 6)
                             | ((promo & 3) << 12) | ((flag & 3) << 14));
}

constexpr int move_from(Move m)  { return m & 0x3F; }
constexpr int move_to(Move m)    { return (m >> 6) & 0x3F; }
constexpr int move_flag(Move m)  { return (m >> 14) & 3; }
constexpr PieceType move_promo(Move m) {
    // promotion type stored as: 0->KNIGHT,1->BISHOP,2->ROOK,3->QUEEN
    return static_cast<PieceType>(KNIGHT + ((m >> 12) & 3));
}

constexpr int FLAG_NORMAL   = 0;
constexpr int FLAG_PROMO    = 1;
constexpr int FLAG_EP       = 2;
constexpr int FLAG_CASTLE   = 3;

// UCI notation ("e2e4", "a7a8q").
std::string move_to_uci(Move m);
Move uci_to_move(const std::string& s, class Board& board);

struct ScoredMove {
    Move move;
    int  score;
};

struct MoveList {
    ScoredMove moves[MAX_MOVES];
    int size = 0;

    void add(Move m, int score = 0) {
        moves[size].move = m;
        moves[size].score = score;
        ++size;
    }
    void clear() { size = 0; }
};

} // namespace jaishi
