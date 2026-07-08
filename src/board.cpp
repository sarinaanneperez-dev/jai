#include "board.h"
#include "movegen.h"
#include "zobrist.h"
#include <sstream>
#include <cctype>
#include <iostream>   // for debug prints

namespace jaishi {

// Castling rights removed when a piece moves to / from these squares.
static const int CASTLING_MASK[64] = {
    13,15,15,15,12,15,15,14,
    15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,
     7,15,15,15, 3,15,15,11,
};

Board::Board() {
    reset();
}

void Board::reset() {
    for (auto& x : piece_bb) x = 0;
    color_bb[0] = color_bb[1] = 0;
    all_bb = 0;
    for (int i = 0; i < 64; ++i) mailbox[i] = NO_PIECE;
    side_to_move = WHITE;
    castling_rights = 0;
    ep_square = NO_SQUARE;
    halfmove_clock = 0;
    fullmove_number = 1;
    zobrist_key = 0;
    history_size = 0;
    undo_size = 0;
}

void Board::put_piece(Piece p, int sq) {
    mailbox[sq] = p;
    piece_bb[p] |= bit(sq);
    color_bb[color_of(p)] |= bit(sq);
    all_bb |= bit(sq);
    zobrist_key ^= ZOBRIST.piece[p][sq];
}
void Board::remove_piece(int sq) {
    Piece p = mailbox[sq];
    if (p == NO_PIECE) return;
    piece_bb[p] &= ~bit(sq);
    color_bb[color_of(p)] &= ~bit(sq);
    all_bb &= ~bit(sq);
    zobrist_key ^= ZOBRIST.piece[p][sq];
    mailbox[sq] = NO_PIECE;
}
void Board::move_piece(int from, int to) {
    Piece p = mailbox[from];
    U64 m = bit(from) | bit(to);
    piece_bb[p] ^= m;
    color_bb[color_of(p)] ^= m;
    all_bb ^= m;
    zobrist_key ^= ZOBRIST.piece[p][from] ^ ZOBRIST.piece[p][to];
    mailbox[from] = NO_PIECE;
    mailbox[to]   = p;
}

int Board::king_square(Color c) const {
    U64 k = piece_bb[make_piece(c, KING)];
    return k ? lsb(k) : NO_SQUARE;
}

U64 Board::attackers_to(int sq, U64 occ) const {
    U64 att = 0;
    att |= PAWN_ATTACKS[BLACK][sq] & piece_bb[W_PAWN];
    att |= PAWN_ATTACKS[WHITE][sq] & piece_bb[B_PAWN];
    att |= KNIGHT_ATTACKS[sq] & (piece_bb[W_KNIGHT] | piece_bb[B_KNIGHT]);
    att |= KING_ATTACKS[sq]   & (piece_bb[W_KING]   | piece_bb[B_KING]);
    U64 bishops_q = piece_bb[W_BISHOP] | piece_bb[B_BISHOP] | piece_bb[W_QUEEN] | piece_bb[B_QUEEN];
    U64 rooks_q   = piece_bb[W_ROOK]   | piece_bb[B_ROOK]   | piece_bb[W_QUEEN] | piece_bb[B_QUEEN];
    att |= bishop_attacks(sq, occ) & bishops_q;
    att |= rook_attacks(sq, occ)   & rooks_q;
    return att;
}

bool Board::square_attacked(int sq, Color by, U64 occ) const {
    if (PAWN_ATTACKS[by ^ 1][sq] & piece_bb[make_piece(by, PAWN)]) return true;
    if (KNIGHT_ATTACKS[sq] & piece_bb[make_piece(by, KNIGHT)])     return true;
    if (KING_ATTACKS[sq]   & piece_bb[make_piece(by, KING)])       return true;
    U64 bq = piece_bb[make_piece(by, BISHOP)] | piece_bb[make_piece(by, QUEEN)];
    if (bishop_attacks(sq, occ) & bq) return true;
    U64 rq = piece_bb[make_piece(by, ROOK)]   | piece_bb[make_piece(by, QUEEN)];
    if (rook_attacks(sq, occ) & rq)   return true;
    return false;
}

bool Board::in_check(Color c) const {
    int ks = king_square(c);
    if (ks == NO_SQUARE) return false;
    return square_attacked(ks, static_cast<Color>(c ^ 1), all_bb);
}

U64 Board::compute_key() const {
    U64 k = 0;
    for (int p = 0; p < 12; ++p) {
        U64 bb = piece_bb[p];
        while (bb) {
            int sq = pop_lsb(bb);
            k ^= ZOBRIST.piece[p][sq];
        }
    }
    k ^= ZOBRIST.castling[castling_rights & 15];
    if (ep_square != NO_SQUARE) k ^= ZOBRIST.en_passant[file_of(ep_square)];
    if (side_to_move == BLACK) k ^= ZOBRIST.side_to_move;
    return k;
}

// -------- FEN parsing --------
bool Board::set_from_fen(const std::string& fen) {
    reset();
    std::istringstream iss(fen);
    std::string boardStr, stmStr, castleStr, epStr;
    int hm = 0, fm = 1;
    if (!(iss >> boardStr >> stmStr >> castleStr >> epStr)) return false;
    iss >> hm >> fm;

    int r = 7, f = 0;
    for (char c : boardStr) {
        if (c == '/') { r--; f = 0; continue; }
        if (std::isdigit(static_cast<unsigned char>(c))) { f += c - '0'; continue; }
        Piece p = NO_PIECE;
        switch (c) {
            case 'P': p = W_PAWN; break; case 'N': p = W_KNIGHT; break;
            case 'B': p = W_BISHOP; break; case 'R': p = W_ROOK; break;
            case 'Q': p = W_QUEEN; break; case 'K': p = W_KING; break;
            case 'p': p = B_PAWN; break; case 'n': p = B_KNIGHT; break;
            case 'b': p = B_BISHOP; break; case 'r': p = B_ROOK; break;
            case 'q': p = B_QUEEN; break; case 'k': p = B_KING; break;
            default: return false;
        }
        if (r < 0 || r > 7 || f < 0 || f > 7) return false;
        put_piece(p, make_square(f, r));
        f++;
    }
    side_to_move = (stmStr == "w") ? WHITE : BLACK;
    castling_rights = 0;
    for (char c : castleStr) {
        switch (c) {
            case 'K': castling_rights |= WHITE_OO; break;
            case 'Q': castling_rights |= WHITE_OOO; break;
            case 'k': castling_rights |= BLACK_OO; break;
            case 'q': castling_rights |= BLACK_OOO; break;
            case '-': default: break;
        }
    }
    if (epStr != "-" && epStr.size() == 2) {
        ep_square = make_square(epStr[0] - 'a', epStr[1] - '1');
    } else {
        ep_square = NO_SQUARE;
    }
    halfmove_clock  = hm;
    fullmove_number = fm;

    // recompute key from scratch
    zobrist_key = compute_key();
    return true;
}

std::string Board::to_fen() const {
    std::string s;
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            Piece p = mailbox[make_square(f, r)];
            if (p == NO_PIECE) { empty++; continue; }
            if (empty) { s += std::to_string(empty); empty = 0; }
            static const char cs[12] = { 'P','N','B','R','Q','K','p','n','b','r','q','k' };
            s += cs[p];
        }
        if (empty) s += std::to_string(empty);
        if (r) s += '/';
    }
    s += ' ';
    s += (side_to_move == WHITE ? 'w' : 'b');
    s += ' ';
    if (!castling_rights) s += '-';
    else {
        if (castling_rights & WHITE_OO)  s += 'K';
        if (castling_rights & WHITE_OOO) s += 'Q';
        if (castling_rights & BLACK_OO)  s += 'k';
        if (castling_rights & BLACK_OOO) s += 'q';
    }
    s += ' ';
    if (ep_square == NO_SQUARE) s += '-';
    else {
        s += static_cast<char>('a' + file_of(ep_square));
        s += static_cast<char>('1' + rank_of(ep_square));
    }
    s += ' ';
    s += std::to_string(halfmove_clock);
    s += ' ';
    s += std::to_string(fullmove_number);
    return s;
}

// -------- make / undo --------
bool Board::make_move(Move m) {
    // Consistency check before
    if (!is_consistent()) {
        std::cerr << "Board inconsistent BEFORE make_move!\n";
        std::cerr << "FEN: " << to_fen() << "\n";
        return false;
    }

    UndoInfo& u = undo_stack[undo_size++];
    u.key       = zobrist_key;
    u.castling  = castling_rights;
    u.ep_square = ep_square;
    u.halfmove  = halfmove_clock;
    u.move      = m;
    u.captured  = NO_PIECE;

    int from = move_from(m), to = move_to(m);
    int flag = move_flag(m);
    Piece moving = mailbox[from];
    Color us = side_to_move, them = static_cast<Color>(us ^ 1);

    // XOR out old ep / castling
    if (ep_square != NO_SQUARE) zobrist_key ^= ZOBRIST.en_passant[file_of(ep_square)];
    zobrist_key ^= ZOBRIST.castling[castling_rights & 15];

    ep_square = NO_SQUARE;
    halfmove_clock++;

    if (flag == FLAG_EP) {
        int capSq = (us == WHITE) ? to - 8 : to + 8;
        u.captured = mailbox[capSq];
        remove_piece(capSq);
        move_piece(from, to);
        halfmove_clock = 0;
    } else if (flag == FLAG_CASTLE) {
        move_piece(from, to);
        // Move rook
        if (to == G1) move_piece(H1, F1);
        else if (to == C1) move_piece(A1, D1);
        else if (to == G8) move_piece(H8, F8);
        else if (to == C8) move_piece(A8, D8);
    } else {
        if (mailbox[to] != NO_PIECE) {
            u.captured = mailbox[to];
            remove_piece(to);
            halfmove_clock = 0;
        }
        move_piece(from, to);

        if (flag == FLAG_PROMO) {
            remove_piece(to); // remove the pawn we just moved
            put_piece(make_piece(us, move_promo(m)), to);
        }

        if (type_of(moving) == PAWN) {
            halfmove_clock = 0;
            // double push -> set ep
            if ((to ^ from) == 16) {
                int ep = (us == WHITE) ? from + 8 : from - 8;
                ep_square = ep;
            }
        }
    }

    // Update castling rights
    castling_rights &= CASTLING_MASK[from];
    castling_rights &= CASTLING_MASK[to];

    // XOR in new ep / castling
    if (ep_square != NO_SQUARE) zobrist_key ^= ZOBRIST.en_passant[file_of(ep_square)];
    zobrist_key ^= ZOBRIST.castling[castling_rights & 15];

    // Flip side
    zobrist_key ^= ZOBRIST.side_to_move;
    side_to_move = them;
    if (us == BLACK) fullmove_number++;

    // Legality check
    if (in_check(us)) {
        undo_move();
        return false;
    }

    key_history[history_size++] = zobrist_key;

    // Consistency check after
    if (!is_consistent()) {
        std::cerr << "Board inconsistent AFTER make_move!\n";
        std::cerr << "Move: " << move_to_uci(m) << "\n";
        std::cerr << "FEN: " << to_fen() << "\n";
        std::abort(); // abort to catch the first corruption
    }

    return true;
}

void Board::undo_move() {
    if (undo_size == 0) return;
    UndoInfo u = undo_stack[--undo_size];
    Move m = u.move;
    int from = move_from(m), to = move_to(m);
    int flag = move_flag(m);
    Color them = side_to_move;
    Color us = static_cast<Color>(them ^ 1);

    side_to_move = us;
    if (us == BLACK) fullmove_number--;

    if (history_size > 0) history_size--;

    if (flag == FLAG_PROMO) {
        remove_piece(to); // remove promoted piece
        put_piece(make_piece(us, PAWN), to);
    }

    if (flag == FLAG_CASTLE) {
        move_piece(to, from);
        if (to == G1) move_piece(F1, H1);
        else if (to == C1) move_piece(D1, A1);
        else if (to == G8) move_piece(F8, H8);
        else if (to == C8) move_piece(D8, A8);
    } else if (flag == FLAG_EP) {
        move_piece(to, from);
        int capSq = (us == WHITE) ? to - 8 : to + 8;
        put_piece(u.captured, capSq);
    } else {
        move_piece(to, from);
        if (u.captured != NO_PIECE) put_piece(u.captured, to);
    }

    castling_rights = u.castling;
    ep_square       = u.ep_square;
    halfmove_clock  = u.halfmove;
    zobrist_key     = u.key;
}

void Board::make_null_move() {
    UndoInfo& u = undo_stack[undo_size++];
    u.key       = zobrist_key;
    u.castling  = castling_rights;
    u.ep_square = ep_square;
    u.halfmove  = halfmove_clock;
    u.move      = NULL_MOVE;
    u.captured  = NO_PIECE;

    if (ep_square != NO_SQUARE) zobrist_key ^= ZOBRIST.en_passant[file_of(ep_square)];
    ep_square = NO_SQUARE;
    zobrist_key ^= ZOBRIST.side_to_move;
    side_to_move = static_cast<Color>(side_to_move ^ 1);
    halfmove_clock++;
    key_history[history_size++] = zobrist_key;
}

void Board::undo_null_move() {
    UndoInfo u = undo_stack[--undo_size];
    if (history_size > 0) history_size--;
    side_to_move   = static_cast<Color>(side_to_move ^ 1);
    ep_square      = u.ep_square;
    halfmove_clock = u.halfmove;
    zobrist_key    = u.key;
    castling_rights = u.castling;
}

bool Board::is_repetition() const {
    if (history_size < 4) return false;
    int limit = std::min(history_size - 1, halfmove_clock);
    int count = 0;
    for (int i = history_size - 3; i >= history_size - 1 - limit && i >= 0; i -= 2) {
        if (key_history[i] == zobrist_key) { if (++count >= 1) return true; }
    }
    return false;
}

bool Board::insufficient_material() const {
    if (piece_bb[W_PAWN] || piece_bb[B_PAWN]) return false;
    if (piece_bb[W_ROOK] || piece_bb[B_ROOK]) return false;
    if (piece_bb[W_QUEEN] || piece_bb[B_QUEEN]) return false;
    int wN = popcount(piece_bb[W_KNIGHT]);
    int bN = popcount(piece_bb[B_KNIGHT]);
    int wB = popcount(piece_bb[W_BISHOP]);
    int bB = popcount(piece_bb[B_BISHOP]);
    int total = wN + bN + wB + bB;
    if (total <= 1) return true;           // K vs K, K+N, K+B
    if (total == 2 && (wN + bN == 2))      // K+N vs K+N (drawish but not forced)
        return false;
    return false;
}

bool Board::is_consistent() const {
    // Verify that bitboards and mailbox agree on every square
    for (int sq = 0; sq < 64; ++sq) {
        Piece p = mailbox[sq];
        bool in_piece_bb = (p != NO_PIECE) ? (piece_bb[p] & bit(sq)) != 0 : false;
        bool in_color_bb = (p != NO_PIECE) ? (color_bb[color_of(p)] & bit(sq)) != 0 : false;
        bool in_all_bb   = (all_bb & bit(sq)) != 0;

        if (p != NO_PIECE) {
            if (!in_piece_bb || !in_color_bb || !in_all_bb) return false;
        } else {
            if (in_piece_bb || in_color_bb || in_all_bb) return false;
        }
    }

    // Verify total piece count matches bitboards
    int total = 0;
    for (int p = 0; p < 12; ++p) total += popcount(piece_bb[p]);
    if (total != popcount(all_bb)) return false;

    // Verify color counts
    if (popcount(color_bb[WHITE]) + popcount(color_bb[BLACK]) != popcount(all_bb)) return false;

    // Verify exactly one king per side
    if (popcount(piece_bb[W_KING]) != 1 || popcount(piece_bb[B_KING]) != 1) return false;

    // Verify that no piece appears in wrong color's bitboard
    for (int p = 0; p < 12; ++p) {
        Color c = color_of(static_cast<Piece>(p));
        if (piece_bb[p] & ~color_bb[c]) return false;
    }

    return true;
}

} // namespace jaishi
