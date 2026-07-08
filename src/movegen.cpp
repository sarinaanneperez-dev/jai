#include "movegen.h"
#include "board.h"

namespace jaishi {

U64 KNIGHT_ATTACKS[64];
U64 KING_ATTACKS[64];
U64 PAWN_ATTACKS[2][64];

static const int KNIGHT_DELTAS[8][2] = {
    {+1,+2},{+2,+1},{+2,-1},{+1,-2},{-1,-2},{-2,-1},{-2,+1},{-1,+2}
};
static const int KING_DELTAS[8][2] = {
    {+1,0},{-1,0},{0,+1},{0,-1},{+1,+1},{+1,-1},{-1,+1},{-1,-1}
};

void movegen_init() {
    for (int sq = 0; sq < 64; ++sq) {
        int f = file_of(sq), r = rank_of(sq);
        U64 kn = 0, kg = 0;
        for (auto& d : KNIGHT_DELTAS) {
            int nf = f + d[0], nr = r + d[1];
            if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8) kn |= bit(make_square(nf, nr));
        }
        for (auto& d : KING_DELTAS) {
            int nf = f + d[0], nr = r + d[1];
            if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8) kg |= bit(make_square(nf, nr));
        }
        KNIGHT_ATTACKS[sq] = kn;
        KING_ATTACKS[sq]   = kg;

        // Pawn attacks
        U64 wa = 0, ba = 0;
        if (f > 0 && r < 7) wa |= bit(make_square(f - 1, r + 1));
        if (f < 7 && r < 7) wa |= bit(make_square(f + 1, r + 1));
        if (f > 0 && r > 0) ba |= bit(make_square(f - 1, r - 1));
        if (f < 7 && r > 0) ba |= bit(make_square(f + 1, r - 1));
        PAWN_ATTACKS[WHITE][sq] = wa;
        PAWN_ATTACKS[BLACK][sq] = ba;
    }
}

static U64 ray(int sq, int df, int dr, U64 occ) {
    U64 out = 0;
    int f = file_of(sq) + df;
    int r = rank_of(sq) + dr;
    while (f >= 0 && f < 8 && r >= 0 && r < 8) {
        int s = make_square(f, r);
        out |= bit(s);
        if (occ & bit(s)) break;
        f += df; r += dr;
    }
    return out;
}

U64 bishop_attacks(int sq, U64 occ) {
    return ray(sq,+1,+1,occ) | ray(sq,+1,-1,occ) | ray(sq,-1,+1,occ) | ray(sq,-1,-1,occ);
}
U64 rook_attacks(int sq, U64 occ) {
    return ray(sq,+1,0,occ) | ray(sq,-1,0,occ) | ray(sq,0,+1,occ) | ray(sq,0,-1,occ);
}

// ---------- move generation ----------

static void add_pawn_moves(int from, int to, int flag, MoveList& list, bool promo) {
    if (promo) {
        for (int p = 0; p < 4; ++p) {
            list.add(static_cast<Move>((from & 0x3F) | ((to & 0x3F) << 6)
                                       | (p << 12) | (FLAG_PROMO << 14)));
        }
    } else {
        list.add(make_move(from, to, flag));
    }
}

static void gen_pawns(Board& b, MoveList& list, bool captures_only) {
    Color us = b.side_to_move, them = static_cast<Color>(us ^ 1);
    U64 pawns = b.pieces(us, PAWN);
    U64 empty = ~b.all_bb;
    U64 enemy = b.color_bb[them];
    int forward = (us == WHITE) ? 8 : -8;
    U64 rank3 = (us == WHITE) ? RANK_3 : RANK_6;
    U64 promoRank = (us == WHITE) ? RANK_8 : RANK_1;

    U64 tmp = pawns;
    while (tmp) {
        int from = pop_lsb(tmp);
        int fr = rank_of(from);

        // Captures
        U64 att = PAWN_ATTACKS[us][from] & enemy;
        while (att) {
            int to = pop_lsb(att);
            bool promo = (bit(to) & promoRank) != 0;
            add_pawn_moves(from, to, FLAG_NORMAL, list, promo);
        }
        // En passant
        if (b.ep_square != NO_SQUARE) {
            if (PAWN_ATTACKS[us][from] & bit(b.ep_square)) {
                list.add(make_move(from, b.ep_square, FLAG_EP));
            }
        }

        if (captures_only) {
            // Also generate promotions on push (they are "noisy")
            int to1 = from + forward;
            if (to1 >= 0 && to1 < 64 && (bit(to1) & promoRank) && (empty & bit(to1))) {
                add_pawn_moves(from, to1, FLAG_NORMAL, list, true);
            }
            continue;
        }

        int to1 = from + forward;
        if (to1 < 0 || to1 >= 64) continue;
        if (empty & bit(to1)) {
            bool promo = (bit(to1) & promoRank) != 0;
            add_pawn_moves(from, to1, FLAG_NORMAL, list, promo);
            // Double push
            if (bit(from) & (us == WHITE ? RANK_2 : RANK_7)) {
                int to2 = to1 + forward;
                if ((empty & bit(to2)) && (bit(to1) & rank3 || true /*intermediate empty*/)) {
                    (void)rank3;
                    if (empty & bit(to2)) list.add(make_move(from, to2, FLAG_NORMAL));
                }
            }
        }
        (void)fr;
    }
}

static void gen_piece(Board& b, MoveList& list, PieceType pt, bool captures_only) {
    Color us = b.side_to_move;
    U64 bb = b.pieces(us, pt);
    U64 own = b.color_bb[us];
    U64 target = captures_only ? b.color_bb[us ^ 1] : ~own;

    while (bb) {
        int from = pop_lsb(bb);
        U64 att = 0;
        switch (pt) {
            case KNIGHT: att = KNIGHT_ATTACKS[from]; break;
            case BISHOP: att = bishop_attacks(from, b.all_bb); break;
            case ROOK:   att = rook_attacks(from, b.all_bb); break;
            case QUEEN:  att = queen_attacks(from, b.all_bb); break;
            case KING:   att = KING_ATTACKS[from]; break;
            default: break;
        }
        att &= target;
        while (att) {
            int to = pop_lsb(att);
            list.add(make_move(from, to, FLAG_NORMAL));
        }
    }
}

static void gen_castling(Board& b, MoveList& list) {
    Color us = b.side_to_move;
    if (b.in_check()) return;
    if (us == WHITE) {
        if ((b.castling_rights & WHITE_OO)
            && !(b.all_bb & (bit(F1) | bit(G1)))
            && !b.square_attacked(F1, BLACK)
            && !b.square_attacked(G1, BLACK)) {
            list.add(make_move(E1, G1, FLAG_CASTLE));
        }
        if ((b.castling_rights & WHITE_OOO)
            && !(b.all_bb & (bit(D1) | bit(C1) | bit(B1)))
            && !b.square_attacked(D1, BLACK)
            && !b.square_attacked(C1, BLACK)) {
            list.add(make_move(E1, C1, FLAG_CASTLE));
        }
    } else {
        if ((b.castling_rights & BLACK_OO)
            && !(b.all_bb & (bit(F8) | bit(G8)))
            && !b.square_attacked(F8, WHITE)
            && !b.square_attacked(G8, WHITE)) {
            list.add(make_move(E8, G8, FLAG_CASTLE));
        }
        if ((b.castling_rights & BLACK_OOO)
            && !(b.all_bb & (bit(D8) | bit(C8) | bit(B8)))
            && !b.square_attacked(D8, WHITE)
            && !b.square_attacked(C8, WHITE)) {
            list.add(make_move(E8, C8, FLAG_CASTLE));
        }
    }
}

void generate_all_moves(Board& b, MoveList& list) {
    list.clear();
    gen_pawns(b, list, false);
    gen_piece(b, list, KNIGHT, false);
    gen_piece(b, list, BISHOP, false);
    gen_piece(b, list, ROOK,   false);
    gen_piece(b, list, QUEEN,  false);
    gen_piece(b, list, KING,   false);
    gen_castling(b, list);
}

void generate_captures(Board& b, MoveList& list) {
    list.clear();
    gen_pawns(b, list, true);
    gen_piece(b, list, KNIGHT, true);
    gen_piece(b, list, BISHOP, true);
    gen_piece(b, list, ROOK,   true);
    gen_piece(b, list, QUEEN,  true);
    gen_piece(b, list, KING,   true);
}

} // namespace jaishi
