#pragma once
#include <cstdint>
#include <array>

namespace jaishi {

using U64 = std::uint64_t;
using U32 = std::uint32_t;
using U16 = std::uint16_t;
using U8  = std::uint8_t;

// Colors
enum Color : int { WHITE = 0, BLACK = 1, NO_COLOR = 2 };
constexpr int NUM_COLORS = 2;

// Piece types
enum PieceType : int {
    PAWN = 0, KNIGHT = 1, BISHOP = 2, ROOK = 3, QUEEN = 4, KING = 5, NO_PIECE_TYPE = 6
};
constexpr int NUM_PIECE_TYPES = 6;

// Pieces (color * 6 + type). NO_PIECE = 12.
enum Piece : int {
    W_PAWN=0, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN,   B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    NO_PIECE = 12
};

constexpr Piece make_piece(Color c, PieceType pt) {
    return static_cast<Piece>(c * 6 + pt);
}
constexpr Color color_of(Piece p) { return static_cast<Color>(p / 6); }
constexpr PieceType type_of(Piece p) { return static_cast<PieceType>(p % 6); }

// Squares (a1 = 0 .. h8 = 63)
enum Square : int {
    A1=0, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NO_SQUARE = 64
};
constexpr int NUM_SQUARES = 64;

constexpr int file_of(int sq) { return sq & 7; }
constexpr int rank_of(int sq) { return sq >> 3; }
constexpr int make_square(int file, int rank) { return rank * 8 + file; }

// Bitboard helpers
constexpr U64 bit(int sq) { return U64(1) << sq; }

// Castling rights (bit flags)
enum CastlingRight : int {
    WHITE_OO  = 1,
    WHITE_OOO = 2,
    BLACK_OO  = 4,
    BLACK_OOO = 8,
    ALL_CASTLING = 15
};

// Files / ranks
constexpr U64 FILE_A = 0x0101010101010101ULL;
constexpr U64 FILE_B = FILE_A << 1;
constexpr U64 FILE_C = FILE_A << 2;
constexpr U64 FILE_D = FILE_A << 3;
constexpr U64 FILE_E = FILE_A << 4;
constexpr U64 FILE_F = FILE_A << 5;
constexpr U64 FILE_G = FILE_A << 6;
constexpr U64 FILE_H = FILE_A << 7;

constexpr U64 RANK_1 = 0xFFULL;
constexpr U64 RANK_2 = RANK_1 << (8 * 1);
constexpr U64 RANK_3 = RANK_1 << (8 * 2);
constexpr U64 RANK_4 = RANK_1 << (8 * 3);
constexpr U64 RANK_5 = RANK_1 << (8 * 4);
constexpr U64 RANK_6 = RANK_1 << (8 * 5);
constexpr U64 RANK_7 = RANK_1 << (8 * 6);
constexpr U64 RANK_8 = RANK_1 << (8 * 7);

// Search / evaluation
constexpr int MAX_PLY = 128;
constexpr int MAX_MOVES = 256;
constexpr int INF_SCORE = 32000;
constexpr int MATE_SCORE = 31000;
constexpr int MATE_IN_MAX = MATE_SCORE - MAX_PLY;
constexpr int DRAW_SCORE = 0;

// Piece values (midgame). Used for MVV-LVA and SEE too.
constexpr int PIECE_VALUE_MG[6] = { 82, 337, 365, 477, 1025, 20000 };
constexpr int PIECE_VALUE_EG[6] = { 94, 281, 297, 512,  936, 20000 };
constexpr int SEE_PIECE_VALUE[7] = { 100, 320, 330, 500, 900, 20000, 0 };

// Starting position FEN
inline constexpr const char* START_FEN =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

} // namespace jaishi
