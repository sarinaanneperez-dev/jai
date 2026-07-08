#include "evaluation.h"
#include "board.h"
#include "movegen.h"

namespace jaishi {

// Piece-square tables (from White's perspective, a1 = 0). Values in centipawns.
// Simple, hand-picked tables (PeSTO-style abridged).
static const int PST_MG[6][64] = {
    // Pawn
    {  0,  0,  0,  0,  0,  0,  0,  0,
       5, 10, 10,-20,-20, 10, 10,  5,
       5, -5,-10,  0,  0,-10, -5,  5,
       0,  0,  0, 20, 20,  0,  0,  0,
       5,  5, 10, 25, 25, 10,  5,  5,
      10, 10, 20, 30, 30, 20, 10, 10,
      50, 50, 50, 50, 50, 50, 50, 50,
       0,  0,  0,  0,  0,  0,  0,  0 },
    // Knight
    {-50,-40,-30,-30,-30,-30,-40,-50,
     -40,-20,  0,  5,  5,  0,-20,-40,
     -30,  5, 10, 15, 15, 10,  5,-30,
     -30,  0, 15, 20, 20, 15,  0,-30,
     -30,  5, 15, 20, 20, 15,  5,-30,
     -30,  0, 10, 15, 15, 10,  0,-30,
     -40,-20,  0,  0,  0,  0,-20,-40,
     -50,-40,-30,-30,-30,-30,-40,-50 },
    // Bishop
    {-20,-10,-10,-10,-10,-10,-10,-20,
     -10,  5,  0,  0,  0,  0,  5,-10,
     -10, 10, 10, 10, 10, 10, 10,-10,
     -10,  0, 10, 10, 10, 10,  0,-10,
     -10,  5,  5, 10, 10,  5,  5,-10,
     -10,  0,  5, 10, 10,  5,  0,-10,
     -10,  0,  0,  0,  0,  0,  0,-10,
     -20,-10,-10,-10,-10,-10,-10,-20 },
    // Rook
    {  0,  0,  0,  5,  5,  0,  0,  0,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
       5, 10, 10, 10, 10, 10, 10,  5,
       0,  0,  0,  0,  0,  0,  0,  0 },
    // Queen
    {-20,-10,-10, -5, -5,-10,-10,-20,
     -10,  0,  5,  0,  0,  0,  0,-10,
     -10,  5,  5,  5,  5,  5,  0,-10,
       0,  0,  5,  5,  5,  5,  0, -5,
      -5,  0,  5,  5,  5,  5,  0, -5,
     -10,  0,  5,  5,  5,  5,  0,-10,
     -10,  0,  0,  0,  0,  0,  0,-10,
     -20,-10,-10, -5, -5,-10,-10,-20 },
    // King (midgame)
    { 20, 30, 10,  0,  0, 10, 30, 20,
      20, 20,  0,  0,  0,  0, 20, 20,
     -10,-20,-20,-20,-20,-20,-20,-10,
     -20,-30,-30,-40,-40,-30,-30,-20,
     -30,-40,-40,-50,-50,-40,-40,-30,
     -30,-40,-40,-50,-50,-40,-40,-30,
     -30,-40,-40,-50,-50,-40,-40,-30,
     -30,-40,-40,-50,-50,-40,-40,-30 }
};
static const int PST_EG[6][64] = {
    // Pawn (endgame — push passers)
    {  0,  0,  0,  0,  0,  0,  0,  0,
      10, 10, 10, 10, 10, 10, 10, 10,
      10, 10, 10, 10, 10, 10, 10, 10,
      20, 20, 20, 20, 20, 20, 20, 20,
      30, 30, 30, 30, 30, 30, 30, 30,
      50, 50, 50, 50, 50, 50, 50, 50,
      80, 80, 80, 80, 80, 80, 80, 80,
       0,  0,  0,  0,  0,  0,  0,  0 },
    // Knight
    {-50,-40,-30,-30,-30,-30,-40,-50,
     -40,-20,  0,  0,  0,  0,-20,-40,
     -30,  0, 10, 15, 15, 10,  0,-30,
     -30,  5, 15, 20, 20, 15,  5,-30,
     -30,  0, 15, 20, 20, 15,  0,-30,
     -30,  5, 10, 15, 15, 10,  5,-30,
     -40,-20,  0,  5,  5,  0,-20,-40,
     -50,-40,-30,-30,-30,-30,-40,-50 },
    // Bishop
    {-20,-10,-10,-10,-10,-10,-10,-20,
     -10,  0,  0,  0,  0,  0,  0,-10,
     -10,  0,  5, 10, 10,  5,  0,-10,
     -10,  5,  5, 10, 10,  5,  5,-10,
     -10,  0, 10, 10, 10, 10,  0,-10,
     -10, 10, 10, 10, 10, 10, 10,-10,
     -10,  5,  0,  0,  0,  0,  5,-10,
     -20,-10,-10,-10,-10,-10,-10,-20 },
    // Rook
    {  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,
       5,  5,  5,  5,  5,  5,  5,  5,
       0,  0,  0,  0,  0,  0,  0,  0 },
    // Queen
    {-20,-10,-10, -5, -5,-10,-10,-20,
     -10,  0,  0,  0,  0,  0,  0,-10,
     -10,  0,  5,  5,  5,  5,  0,-10,
      -5,  0,  5,  5,  5,  5,  0, -5,
       0,  0,  5,  5,  5,  5,  0, -5,
     -10,  5,  5,  5,  5,  5,  0,-10,
     -10,  0,  5,  0,  0,  0,  0,-10,
     -20,-10,-10, -5, -5,-10,-10,-20 },
    // King (endgame — activate)
    {-50,-30,-30,-30,-30,-30,-30,-50,
     -30,-30,  0,  0,  0,  0,-30,-30,
     -30,-10, 20, 30, 30, 20,-10,-30,
     -30,-10, 30, 40, 40, 30,-10,-30,
     -30,-10, 30, 40, 40, 30,-10,-30,
     -30,-10, 20, 30, 30, 20,-10,-30,
     -30,-20,-10,  0,  0,-10,-20,-30,
     -50,-40,-30,-20,-20,-30,-40,-50 }
};

// Phase weights per piece type (Fruit / PeSTO style)
static const int PHASE_WEIGHT[6] = { 0, 1, 1, 2, 4, 0 };
static constexpr int PHASE_TOTAL = 24;

static inline int mirror_sq(int sq) { return sq ^ 56; }

int piece_value_mg(int pt) { return PIECE_VALUE_MG[pt]; }

int game_phase(const Board& b) {
    int phase = 0;
    for (int pt = KNIGHT; pt <= QUEEN; ++pt) {
        phase += PHASE_WEIGHT[pt] *
                 (__builtin_popcountll(b.piece_bb[make_piece(WHITE, static_cast<PieceType>(pt))])
                + __builtin_popcountll(b.piece_bb[make_piece(BLACK, static_cast<PieceType>(pt))]));
    }
    if (phase > PHASE_TOTAL) phase = PHASE_TOTAL;
    return phase;
}

// Simple pawn structure — isolated, doubled, passed
static int pawn_structure(const Board& b, Color c) {
    U64 our = b.pieces(c, PAWN);
    U64 their = b.pieces(static_cast<Color>(c ^ 1), PAWN);
    int score = 0;

    // file masks
    U64 file_bb[8] = {
        FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H
    };
    int per_file[8] = { 0 };
    U64 tmp = our;
    while (tmp) {
        int sq = pop_lsb(tmp);
        per_file[file_of(sq)]++;
    }

    for (int f = 0; f < 8; ++f) {
        int n = per_file[f];
        if (n == 0) continue;
        if (n > 1) score -= 15 * (n - 1);  // doubled
        // isolated
        U64 neighbor = 0;
        if (f > 0) neighbor |= file_bb[f - 1];
        if (f < 7) neighbor |= file_bb[f + 1];
        if (!(our & neighbor)) score -= 12 * n;
    }

    // Passed pawns
    tmp = our;
    while (tmp) {
        int sq = pop_lsb(tmp);
        int f = file_of(sq), r = rank_of(sq);
        U64 mask = file_bb[f];
        if (f > 0) mask |= file_bb[f - 1];
        if (f < 7) mask |= file_bb[f + 1];
        // squares ahead of this pawn
        U64 ahead;
        if (c == WHITE) ahead = mask & ~((U64(1) << ((r + 1) * 8)) - 1);
        else            ahead = mask &  ((U64(1) << (r * 8)) - 1);
        if (!(their & ahead)) {
            int rel = (c == WHITE) ? r : (7 - r);
            static const int passed_bonus[8] = { 0, 5, 10, 20, 35, 60, 100, 0 };
            score += passed_bonus[rel];
        }
    }
    return score;
}

static int king_safety(const Board& b, Color c) {
    int ks = b.king_square(c);
    if (ks == NO_SQUARE) return 0;
    U64 shield_area = KING_ATTACKS[ks];
    U64 our_pawns = b.pieces(c, PAWN);
    int shield = __builtin_popcountll(shield_area & our_pawns) * 8;
    return shield;
}

static int mobility_and_pieces(const Board& b, Color c) {
    int score = 0;
    U64 occ = b.all_bb;
    U64 own = b.color_bb[c];

    // Bishop pair
    if (__builtin_popcountll(b.pieces(c, BISHOP)) >= 2) score += 30;

    // Rook on (semi-)open file
    U64 rooks = b.pieces(c, ROOK);
    U64 own_p   = b.pieces(c, PAWN);
    U64 their_p = b.pieces(static_cast<Color>(c ^ 1), PAWN);
    U64 file_bb[8] = { FILE_A,FILE_B,FILE_C,FILE_D,FILE_E,FILE_F,FILE_G,FILE_H };
    U64 tmp = rooks;
    while (tmp) {
        int sq = pop_lsb(tmp);
        U64 fm = file_bb[file_of(sq)];
        if (!(fm & own_p)) {
            if (!(fm & their_p)) score += 20;   // open
            else                 score += 10;   // semi-open
        }
    }

    // Mobility (knights, bishops, rooks, queens)
    U64 kn = b.pieces(c, KNIGHT);
    while (kn) { int sq = pop_lsb(kn); score += 4 * __builtin_popcountll(KNIGHT_ATTACKS[sq] & ~own); }
    U64 bi = b.pieces(c, BISHOP);
    while (bi) { int sq = pop_lsb(bi); score += 3 * __builtin_popcountll(bishop_attacks(sq, occ) & ~own); }
    U64 ro = b.pieces(c, ROOK);
    while (ro) { int sq = pop_lsb(ro); score += 2 * __builtin_popcountll(rook_attacks(sq, occ) & ~own); }
    U64 qu = b.pieces(c, QUEEN);
    while (qu) { int sq = pop_lsb(qu); score += 1 * __builtin_popcountll(queen_attacks(sq, occ) & ~own); }

    return score;
}

int evaluate(const Board& b) {
    int mg[2] = { 0, 0 };
    int eg[2] = { 0, 0 };

    for (int c = 0; c < 2; ++c) {
        for (int pt = 0; pt < 6; ++pt) {
            Piece p = make_piece(static_cast<Color>(c), static_cast<PieceType>(pt));
            U64 bb = b.piece_bb[p];
            while (bb) {
                int sq = pop_lsb(bb);
                int idx = (c == WHITE) ? mirror_sq(sq) : sq;
                mg[c] += PIECE_VALUE_MG[pt] + PST_MG[pt][idx];
                eg[c] += PIECE_VALUE_EG[pt] + PST_EG[pt][idx];
            }
        }
    }

    int mg_score = mg[WHITE] - mg[BLACK];
    int eg_score = eg[WHITE] - eg[BLACK];

    // Pawn structure + king safety + mobility (side-independent additions)
    int extra_w = pawn_structure(b, WHITE) + king_safety(b, WHITE) + mobility_and_pieces(b, WHITE);
    int extra_b = pawn_structure(b, BLACK) + king_safety(b, BLACK) + mobility_and_pieces(b, BLACK);
    mg_score += (extra_w - extra_b);
    eg_score += (extra_w - extra_b);

    int phase = game_phase(b);
    int score = (mg_score * phase + eg_score * (PHASE_TOTAL - phase)) / PHASE_TOTAL;

    // Tempo
    score += (b.side_to_move == WHITE ? 8 : -8);

    return (b.side_to_move == WHITE) ? score : -score;
}

} // namespace jaishi
