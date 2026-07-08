#pragma once
#include "board.h"

namespace jaishi {

// Initializes all engine subsystems (zobrist, movegen tables, TT).
// Safe to call multiple times.
void engine_init();

// UCI protocol loop. Reads from std::cin, writes to std::cout. Returns on `quit`.
void uci_loop();

} // namespace jaishi
