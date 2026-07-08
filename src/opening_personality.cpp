#include "opening_personality.h"
#include "board.h"
#include "movegen.h"
#include <cstdlib>

namespace jaishi {

OpeningPersonality PERSONALITY;

// Long diagonals a1-h8 and h1-a8
static constexpr U64 LONG_DIAG_1 = 0x8040201008040201ULL; // a1..h8
static constexpr U64 LONG_DIAG_2 = 0x0102040810204080ULL; // h1..a8

int OpeningPersonality::score_move(const Board& b, Move m) const {
    if (!enabled) return 0;

    int score = 0;
    int from = move_from(m);
    int to   = move_to(m);
    Piece p  = b.piece_on(from);
    if (p == NO_PIECE) return 0;
    Color us = color_of(p);
    PieceType pt = type_of(p);

    // --- Double fianchetto preference ---
    // Reward b2/g2 (b7/g7) pawn pushes to b3/g3 (b6/g6) and bishop development
    // to the long diagonal.
    if (pt == PAWN) {
        if (us == WHITE) {
            if ((from == B2 && to == B3) || (from == G2 && to == G3)) score += 18;
        } else {
            if ((from == B7 && to == B6) || (from == G7 && to == G6)) score += 18;
        }
    }
    if (pt == BISHOP) {
        int rel_to = (us == WHITE) ? to : (to ^ 56);
        if (rel_to == B2 || rel_to == G2) score += 22;
        // Long diagonal presence
        U64 diag = LONG_DIAG_1 | LONG_DIAG_2;
        if (bit(to) & diag) score += 10;
    }

    // --- Kingside attack preference ---
    // Reward pushes / moves that head toward the enemy king's side.
    int enemy_king = b.king_square(static_cast<Color>(us ^ 1));
    if (enemy_king != NO_SQUARE) {
        int king_file = file_of(enemy_king);
        int to_file   = file_of(to);
        int dist      = std::abs(king_file - to_file);
        if (dist <= 2) score += 6;
        if (pt == PAWN && dist <= 2) {
            int rel_to = (us == WHITE) ? rank_of(to) : (7 - rank_of(to));
            if (rel_to >= 4) score += 8; // pawn storm
        }
    }

    // --- Space / central control ---
    static constexpr U64 CENTER = (bit(D4) | bit(E4) | bit(D5) | bit(E5));
    static constexpr U64 EXT_CENTER =
        (bit(C3)|bit(D3)|bit(E3)|bit(F3)|
         bit(C4)|bit(D4)|bit(E4)|bit(F4)|
         bit(C5)|bit(D5)|bit(E5)|bit(F5)|
         bit(C6)|bit(D6)|bit(E6)|bit(F6));
    if (bit(to) & CENTER)       score += 10;
    else if (bit(to) & EXT_CENTER) score += 4;

    // --- Initiative: reward developing minor pieces and checks ---
    if ((pt == KNIGHT || pt == BISHOP) && rank_of(from) == (us == WHITE ? 0 : 7))
        score += 12;

    // --- Long diagonal preference for queen/bishop ---
    if (pt == BISHOP || pt == QUEEN) {
        if (bit(to) & (LONG_DIAG_1 | LONG_DIAG_2)) score += 6;
    }

    // --- Outpost preference for knights ---
    // A knight on rank 4/5/6 (relative) not attackable by an enemy pawn.
    if (pt == KNIGHT) {
        int rel_r = (us == WHITE) ? rank_of(to) : (7 - rank_of(to));
        if (rel_r >= 3 && rel_r <= 5) {
            U64 enemy_pawns = b.pieces(static_cast<Color>(us ^ 1), PAWN);
            U64 attackers = PAWN_ATTACKS[us][to] & enemy_pawns;
            if (!attackers) score += 14;
        }
    }

    // Cap the personality contribution so it never dominates real evaluation.
    if (score >  strength * 4) score =  strength * 4;
    if (score < -strength * 4) score = -strength * 4;
    return score;
}

} // namespace jaishi
