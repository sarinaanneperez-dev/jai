#include "board.h"
#include "evaluation.h"
#include "uci.h"
#include <iostream>

using namespace jaishi;

int main() {
    engine_init();
    Board b;
    b.set_from_fen(START_FEN);
    int s = evaluate(b);
    std::cout << "[INFO] startpos eval (stm view): " << s << "\n";
    if (s < -50 || s > 50) {
        std::cout << "[FAIL] startpos eval out of sane range\n";
        return 1;
    }

    // Material imbalance: white up a queen.
    b.set_from_fen("4k3/8/8/8/8/8/8/3QK3 w - - 0 1");
    int s2 = evaluate(b);
    std::cout << "[INFO] K+Q vs K eval: " << s2 << "\n";
    if (s2 < 500) {
        std::cout << "[FAIL] expected large positive eval\n";
        return 1;
    }
    std::cout << "[ OK ] evaluation smoke tests\n";
    return 0;
}
