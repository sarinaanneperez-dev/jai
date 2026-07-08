#pragma once
#include "constants.h"
#include "move.h"
#include <string>

namespace jaishi {

struct UndoInfo {
    U64      key;
    Piece    captured;
    int      castling;
    int      ep_square;   // NO_SQUARE if none
    int      halfmove;
    Move     move;
};

class Board {
public:
    // Bitboards, indexed by Piece 0..11
    U64   piece_bb[12] = { 0 };
    U64   color_bb[2]  = { 0, 0 };
    U64   all_bb       = 0;
    Piece mailbox[64];

    Color side_to_move = WHITE;
    int   castling_rights = 0;
    int   ep_square = NO_SQUARE;
    int   halfmove_clock = 0;
    int   fullmove_number = 1;

    U64   zobrist_key = 0;

    // Position history for repetition detection (keys after each move made)
    U64  key_history[1024];
    int  history_size = 0;

    // Undo stack
    UndoInfo undo_stack[1024];
    int      undo_size = 0;

    Board();

    void reset();
    bool set_from_fen(const std::string& fen);
    std::string to_fen() const;

    // Query
    Piece piece_on(int sq) const { return mailbox[sq]; }
    U64   pieces(Color c) const { return color_bb[c]; }
    U64   pieces(Color c, PieceType pt) const { return piece_bb[make_piece(c, pt)]; }
    U64   pieces(PieceType pt) const {
        return piece_bb[make_piece(WHITE, pt)] | piece_bb[make_piece(BLACK, pt)];
    }
    int   king_square(Color c) const;

    // Attack query. Occupancy passed explicitly to allow SEE.
    bool  square_attacked(int sq, Color by, U64 occupancy) const;
    bool  square_attacked(int sq, Color by) const { return square_attacked(sq, by, all_bb); }
    bool  in_check(Color c) const;
    bool  in_check() const { return in_check(side_to_move); }

    U64   attackers_to(int sq, U64 occupancy) const;

    // Make / undo. Returns false if illegal (own king attacked).
    bool  make_move(Move m);
    void  undo_move();

    // Null move.
    void  make_null_move();
    void  undo_null_move();

    // Repetition / 50-move draw
    bool  is_repetition() const;
    bool  is_fifty_move_draw() const { return halfmove_clock >= 100; }
    bool  insufficient_material() const;

    // Rebuild zobrist key from scratch (for verification / setup).
    U64   compute_key() const;

    // Consistency check – returns true if board state is valid.
    bool  is_consistent() const;

private:
    void  put_piece(Piece p, int sq);
    void  remove_piece(int sq);
    void  move_piece(int from, int to);
};

} // namespace jaishi
