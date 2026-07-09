#include "uci.h"
#include "board.h"
#include "movegen.h"
#include "zobrist.h"
#include "search.h"
#include "tt.h"
#include "opening_personality.h"
#include "perft.h"
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <atomic>

namespace jaishi {

static bool initialized = false;

void engine_init() {
    if (initialized) return;
    zobrist_init();
    movegen_init();
    TT.resize_mb(64);
    initialized = true;
}

namespace {

Board g_board;
std::thread g_search_thread;
std::atomic<bool> g_searching{false};

void join_search() {
    if (g_search_thread.joinable()) g_search_thread.join();
}

void handle_uci() {
    std::cout << "id name Jaishi 1.0.0\n";
    std::cout << "id author The Jaishi Authors\n";
    std::cout << "option name Hash type spin default 64 min 1 max 4096\n";
    std::cout << "option name Personality type check default false\n";
    std::cout << "option name PersonalityStrength type spin default 25 min 0 max 100\n";
    std::cout << "uciok" << std::endl;
}

void handle_isready() {
    std::cout << "readyok" << std::endl;
}

void handle_setoption(std::istringstream& iss) {
    std::string tok, name, value;
    // format: setoption name <NAME> value <VALUE>
    iss >> tok; // "name"
    std::string cur;
    while (iss >> tok) {
        if (tok == "value") break;
        if (!name.empty()) name += " ";
        name += tok;
    }
    while (iss >> tok) {
        if (!value.empty()) value += " ";
        value += tok;
    }
    if (name == "Hash") {
        int mb = std::stoi(value);
        TT.resize_mb(mb);
    } else if (name == "Personality") {
        PERSONALITY.enabled = (value == "true" || value == "1");
    } else if (name == "PersonalityStrength") {
        PERSONALITY.strength = std::stoi(value);
    }
}

void handle_position(std::istringstream& iss) {
    // CRITICAL FIX: Stop any active search before modifying the board.
    // The search thread reads g_board concurrently; modifying it while
    // searching causes undefined behavior (corrupted board state).
    if (g_searching.load()) {
        STOP_FLAG.store(true);
        join_search();
        g_searching.store(false);
    }

    std::string tok;
    if (!(iss >> tok)) return;
    if (tok == "startpos") {
        g_board.set_from_fen(START_FEN);
        if (iss >> tok && tok != "moves") return;
    } else if (tok == "fen") {
        std::string fen;
        for (int i = 0; i < 6 && iss >> tok; ++i) {
            if (tok == "moves") break;
            if (!fen.empty()) fen += " ";
            fen += tok;
        }
        g_board.set_from_fen(fen);
        if (tok != "moves") { /* already consumed */ }
    }
    // moves loop
    while (iss >> tok) {
        Move m = uci_to_move(tok, g_board);
        if (m == NULL_MOVE) break;
        if (!g_board.make_move(m)) break;
    }
}

void handle_go(std::istringstream& iss) {
    SearchLimits limits;
    std::string tok;
    while (iss >> tok) {
        if      (tok == "depth")     iss >> limits.depth;
        else if (tok == "movetime")  iss >> limits.movetime_ms;
        else if (tok == "wtime")     iss >> limits.wtime;
        else if (tok == "btime")     iss >> limits.btime;
        else if (tok == "winc")      iss >> limits.winc;
        else if (tok == "binc")      iss >> limits.binc;
        else if (tok == "movestogo") iss >> limits.movestogo;
        else if (tok == "infinite")  limits.infinite = true;
    }

    join_search();
    STOP_FLAG.store(false);
    g_searching.store(true);
    g_search_thread = std::thread([limits] {
        Move best = search_best_move(g_board, limits);
        g_searching.store(false);
        std::cout << "bestmove " << (best == NULL_MOVE ? std::string("0000") : move_to_uci(best))
                  << std::endl;
    });
}

void handle_stop() {
    STOP_FLAG.store(true);
    join_search();
    g_searching.store(false);
}

void handle_perft(std::istringstream& iss) {
    int d = 1;
    iss >> d;
    U64 n = perft_divide(g_board, d);
    std::cout << "perft(" << d << ") = " << n << std::endl;
}

} // namespace

void uci_loop() {
    engine_init();
    g_board.set_from_fen(START_FEN);

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        if      (cmd == "uci")        handle_uci();
        else if (cmd == "isready")    handle_isready();
        else if (cmd == "setoption")  handle_setoption(iss);
        else if (cmd == "ucinewgame") { handle_stop(); TT.clear(); g_board.set_from_fen(START_FEN); }
        else if (cmd == "position")   handle_position(iss);
        else if (cmd == "go")         handle_go(iss);
        else if (cmd == "stop")       handle_stop();
        else if (cmd == "perft")      handle_perft(iss);
        else if (cmd == "d")          std::cout << g_board.to_fen() << std::endl;
        else if (cmd == "quit")       { handle_stop(); break; }
    }
    handle_stop();
}

} // namespace jaishi
