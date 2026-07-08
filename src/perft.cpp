#include "perft.h"
#include "board.h"
#include "movegen.h"
#include <iostream>

namespace jaishi {

U64 perft(Board& b, int depth) {
    if (depth == 0) return 1;
    MoveList list;
    generate_all_moves(b, list);
    U64 nodes = 0;
    if (depth == 1) {
        // Count legal moves directly
        for (int i = 0; i < list.size; ++i) {
            if (b.make_move(list.moves[i].move)) {
                nodes++;
                b.undo_move();
            }
        }
        return nodes;
    }
    for (int i = 0; i < list.size; ++i) {
        if (!b.make_move(list.moves[i].move)) continue;
        nodes += perft(b, depth - 1);
        b.undo_move();
    }
    return nodes;
}

U64 perft_divide(Board& b, int depth) {
    MoveList list;
    generate_all_moves(b, list);
    U64 total = 0;
    for (int i = 0; i < list.size; ++i) {
        Move m = list.moves[i].move;
        if (!b.make_move(m)) continue;
        U64 c = (depth <= 1) ? 1 : perft(b, depth - 1);
        b.undo_move();
        std::cout << move_to_uci(m) << ": " << c << "\n";
        total += c;
    }
    std::cout << "Total: " << total << "\n";
    return total;
}

} // namespace jaishi
