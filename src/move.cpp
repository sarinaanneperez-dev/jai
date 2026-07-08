#include "move.h"
#include "board.h"
#include "movegen.h"
#include <cctype>

namespace jaishi {

static char file_char(int f) { return static_cast<char>('a' + f); }
static char rank_char(int r) { return static_cast<char>('1' + r); }

std::string move_to_uci(Move m) {
    if (m == NULL_MOVE) return "0000";
    int f = move_from(m);
    int t = move_to(m);
    std::string s;
    s.push_back(file_char(file_of(f)));
    s.push_back(rank_char(rank_of(f)));
    s.push_back(file_char(file_of(t)));
    s.push_back(rank_char(rank_of(t)));
    if (move_flag(m) == FLAG_PROMO) {
        static const char pc[4] = { 'n', 'b', 'r', 'q' };
        s.push_back(pc[(m >> 12) & 3]);
    }
    return s;
}

Move uci_to_move(const std::string& s, Board& board) {
    if (s.size() < 4) return NULL_MOVE;
    int ff = s[0] - 'a', fr = s[1] - '1';
    int tf = s[2] - 'a', tr = s[3] - '1';
    if (ff < 0 || ff > 7 || fr < 0 || fr > 7 || tf < 0 || tf > 7 || tr < 0 || tr > 7)
        return NULL_MOVE;
    int from = make_square(ff, fr);
    int to   = make_square(tf, tr);

    MoveList list;
    generate_all_moves(board, list);
    for (int i = 0; i < list.size; ++i) {
        Move m = list.moves[i].move;
        if (move_from(m) != from || move_to(m) != to) continue;
        if (move_flag(m) == FLAG_PROMO) {
            if (s.size() < 5) continue;
            char pc = static_cast<char>(std::tolower(s[4]));
            char want = "nbrq"[(m >> 12) & 3];
            if (pc != want) continue;
        }
        return m;
    }
    return NULL_MOVE;
}

} // namespace jaishi
