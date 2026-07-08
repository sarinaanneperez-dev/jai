# Search

Jaishi uses a fairly standard modern search stack:

- **Iterative deepening** with aspiration windows starting at depth 5.
- **Principal Variation Search** — first move full window, remaining moves scouted with a zero-window and re-searched on fail-high.
- **Transposition table** cutoffs (exact, lower, upper) with mate-score adjustment across plies.
- **Null-move pruning** with `R = 3 + depth/6`, disabled in check and when the side to move has only king + pawns.
- **Late Move Reductions** using `LMR(d, m) = 0.75 + log(d)*log(m)/2.25`, only for quiet, non-check moves after the first three.
- **Reverse futility pruning** at shallow depths.
- **Check extension**.
- **Mate distance pruning**.
- **Quiescence** with stand-pat and SEE pruning of clearly-losing captures.

Move ordering: TT move → MVV/LVA captures → promotions → killers → history.
