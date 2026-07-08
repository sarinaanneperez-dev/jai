# Architecture

Jaishi is organized as a small set of focused modules:

- **Board (`board.{h,cpp}`)** — bitboard position, make/undo, FEN I/O, Zobrist maintenance, repetition history.
- **Move generator (`movegen.{h,cpp}`)** — pseudo-legal move generation using precomputed leaper tables and classical ray-scan slider attacks. Legality is verified in `Board::make_move`.
- **Evaluation (`evaluation.{h,cpp}`)** — tapered mg/eg evaluation with piece-square tables, pawn structure, king safety, mobility, bishop pair, and rook on (semi-)open files.
- **Search (`search.{h,cpp}`)** — iterative deepening, PVS, TT, null-move pruning, LMR, futility, aspiration windows, quiescence with SEE pruning.
- **Transposition table (`tt.{h,cpp}`)** — depth-preferred, always-replace-if-older replacement, mate-score adjusted.
- **History (`history.{h,cpp}`)** — from/to butterfly table + 2-slot killers.
- **Personality (`opening_personality.{h,cpp}`)** — stylistic bias applied only among near-equal candidates.
- **Time manager (`time_manager.{h,cpp}`)** — parses `go` limits and enforces a soft/hard budget.
- **UCI driver (`uci.{h,cpp}`)** — parses stdin, spawns a worker thread for `go`, prints `info` and `bestmove`.

See `search.md`, `evaluation.md`, and `opening_personality.md` for details.
