# Evaluation

Tapered evaluation between midgame (mg) and endgame (eg) using standard phase
weights (N=1, B=1, R=2, Q=4, phase total 24).

Components:

- **Material + PST** — PeSTO-style abbreviated tables for both phases.
- **Pawn structure** — doubled and isolated pawn penalties; passed-pawn bonus scaled by rank.
- **King safety** — pawn shield count around the king (crude but effective at low depths).
- **Mobility** — piece-typed mobility weights (N=4, B=3, R=2, Q=1) over legal destination squares.
- **Bishop pair** — +30 cp.
- **Rook on (semi-)open file** — +20 / +10 cp.
- **Tempo** — small bonus for the side to move.

Returned from the perspective of the side to move.
