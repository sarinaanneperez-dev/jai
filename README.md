# Jaishi

Jaishi is a modern, UCI-compatible chess engine written in C++20.
It is bitboard-based, features a Stockfish-inspired search architecture,
and ships with a **human-style, aggressive personality module** that
biases move choice among near-equal alternatives.

> Play the position, but with style.

---

## Table of contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Building](#building)
- [UCI commands](#uci-commands)
- [Engine personality](#engine-personality)
- [Roadmap](#roadmap)
- [Example games](#example-games)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

Jaishi is an **engine only** — no GUI, no web frontend, no mobile app.
It communicates over stdin/stdout using the Universal Chess Interface
(UCI) protocol, so it plugs into any UCI-compatible GUI such as
CuteChess, Arena, Banksia, or ChessBase.

The project is organized as a modular C++20 codebase: bitboard board
representation, staged move generation, principal-variation search with
modern pruning, a hand-tuned tapered evaluation, and a personality layer
that nudges the engine toward a distinctive playing style.

## Features

**Search**

- Iterative deepening
- Principal Variation Search (PVS)
- Alpha-beta with quiescence
- Transposition table (Zobrist hashing)
- Null-move pruning
- Late move reductions (LMR)
- Futility pruning
- History heuristic + killer moves
- MVV-LVA capture ordering
- Static Exchange Evaluation (SEE)
- Aspiration windows

**Evaluation**

- Tapered (midgame / endgame) evaluation
- Piece-square tables
- Pawn structure (isolated, doubled, passed)
- King safety and pawn shield
- Mobility
- Bishop pair, rook on open file
- Incremental material updates

**Personality**

- Double-fianchetto preference
- Kingside attack bias
- Space and initiative bonuses
- Long-diagonal control
- Outpost detection
- **Never** overrides a clearly better move — only breaks near-ties.

## Architecture

```
+-------------------+     +---------------------+
|    UCI driver     | <-> |   Time manager      |
+---------+---------+     +---------------------+
          |
          v
+-------------------+     +---------------------+
|      Search       | <-> | Transposition table |
| (PVS + pruning)   |     +---------------------+
+---------+---------+
          |
          v
+-------------------+     +---------------------+
|   Move generator  | <-> |    Board (bitboards)|
+---------+---------+     +---------------------+
          |
          v
+-------------------+     +---------------------+
|    Evaluation     | <-> | Opening personality |
+-------------------+     +---------------------+
```

See [`docs/architecture.md`](docs/architecture.md) for the full write-up.

## Building

Requirements: **CMake ≥ 3.16**, a C++20 compiler (GCC 11+, Clang 13+, MSVC 2022).

```bash
# Linux / macOS
./scripts/build.sh

# Windows
scripts\build.bat
```

Or manually:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
./build/jaishi
```

Run the perft self-test:

```bash
./build/jaishi_tests
```

## UCI commands

Jaishi implements the standard UCI command set:

| Command | Description |
|--------|-------------|
| `uci` | Identify engine and options |
| `isready` | Handshake |
| `ucinewgame` | Reset internal state |
| `position [startpos \| fen ...] moves ...` | Set the position |
| `go [depth N] [movetime N] [wtime N btime N winc N binc N]` | Start searching |
| `stop` | Stop the current search |
| `quit` | Exit |
| `setoption name <name> value <v>` | Configure engine |

Custom options:

| Option | Type | Default | Purpose |
|--------|------|---------|---------|
| `Hash` | spin (1..4096 MB) | 64 | Transposition table size |
| `Personality` | check | true | Enable human-style bias |
| `PersonalityStrength` | spin (0..100) | 25 | Bias magnitude in centipawns |

## Engine personality

Jaishi's `OpeningPersonality` module scores each candidate move on a
number of stylistic axes (fianchetto structure, kingside pawn storms,
central space, initiative, long diagonals, outposts). The score is
added to the search evaluation **only when the difference between two
candidate moves is below `PersonalityStrength` centipawns**. This
guarantees Jaishi never plays an objectively worse move for style — it
only breaks ties.

See [`docs/opening_personality.md`](docs/opening_personality.md).

## Roadmap

- [ ] Magic bitboards for slider attacks
- [ ] NNUE evaluation
- [ ] Syzygy tablebase probing
- [ ] Multi-threaded (Lazy SMP) search
- [ ] SPRT-tuned personality weights
- [ ] Pondering
- [ ] Chess960

## Example games

Play against Jaishi in any UCI GUI, or pit it against other engines
using [cutechess-cli](https://github.com/cutechess/cutechess):

```bash
cutechess-cli \
  -engine cmd=./build/jaishi name=Jaishi \
  -engine cmd=stockfish name=Stockfish \
  -each proto=uci tc=40/60 \
  -games 20
```

## Contributing

Pull requests are welcome. Please:

1. Open an issue describing the change first.
2. Keep pull requests focused and self-contained.
3. Run `./build/jaishi_tests` before submitting.
4. Follow the existing code style (4-space indent, `snake_case`
   functions, `PascalCase` types).
5. For search or evaluation changes, include an SPRT log or at minimum a
   short perft / bench regression check.

## License

Jaishi is released under the MIT License. See [`LICENSE`](LICENSE).
