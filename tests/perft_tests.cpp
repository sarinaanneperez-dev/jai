#include "board.h"
#include "perft.h"
#include "uci.h"
#include <iostream>
#include <string>

using namespace jaishi;

struct Case { const char* fen; int depth; U64 expected; };

// Well-known perft positions (Kiwipete etc.). Depths chosen to run fast.
static const Case CASES[] = {
    { START_FEN, 4, 197281ULL },
    { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862ULL },
    { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 4, 43238ULL },
    { "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333ULL },
};

int main() {
    engine_init();
    int failed = 0;
    for (const auto& c : CASES) {
        Board b;
        b.set_from_fen(c.fen);
        U64 got = perft(b, c.depth);
        bool ok = (got == c.expected);
        std::cout << (ok ? "[ OK ] " : "[FAIL] ")
                  << "perft(" << c.depth << ") " << c.fen
                  << "  got=" << got << " expected=" << c.expected << "\n";
        if (!ok) failed++;
    }
    return failed == 0 ? 0 : 1;
}
