#include "board.h"
#include "search.h"
#include "uci.h"
#include <iostream>

using namespace jaishi;

int main() {
    engine_init();
    Board b;
    b.set_from_fen(START_FEN);

    SearchLimits limits;
    limits.depth = 5;
    Move m = search_best_move(b, limits);
    if (m == NULL_MOVE) {
        std::cout << "[FAIL] no move returned at startpos\n";
        return 1;
    }
    std::cout << "[ OK ] startpos best move: " << move_to_uci(m) << "\n";

    // Mate-in-1: white to play, Qh5#. Actually pick a simpler forced mate FEN.
    // Fool's mate final position lookup — use: white to move mate in 1
    // "6k1/5ppp/8/8/8/8/5PPP/R5K1 w - - 0 1" — Ra8#
    b.set_from_fen("6k1/5ppp/8/8/8/8/5PPP/R5K1 w - - 0 1");
    limits.depth = 4;
    m = search_best_move(b, limits);
    std::cout << "[INFO] mate test best: " << move_to_uci(m)
              << " score " << LAST_SEARCH.score << "\n";
    return 0;
}
