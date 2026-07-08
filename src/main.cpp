#include "uci.h"
#include <iostream>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    jaishi::engine_init();
    jaishi::uci_loop();
    return 0;
}
