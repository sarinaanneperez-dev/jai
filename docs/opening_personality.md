# Opening Personality

The `OpeningPersonality` module is what makes Jaishi *Jaishi*.

## Traits scored

| Trait | Description |
|-------|-------------|
| Double-fianchetto | Bonus for `b2/g2` (`b7/g7`) pawn pushes and bishop development to the long diagonals. |
| Kingside attack | Bonus for pieces heading toward the file of the enemy king; extra bonus for advanced pawns on those files. |
| Space / center | Bonus for occupying `d4/e4/d5/e5` and the extended center. |
| Initiative | Bonus for developing minor pieces off the back rank. |
| Long diagonal | Bonus for queen/bishop moves onto `a1-h8` or `h1-a8`. |
| Outposts | Bonus for knights on rank 4–6 that no enemy pawn can attack. |

## The safety guarantee

The style score is **never** applied as part of alpha-beta — the search always
returns the objectively best score. Personality only takes effect at the root,
where Jaishi re-inspects the top candidate moves. Among moves whose evaluation
is within `PersonalityStrength` centipawns of the best, Jaishi picks the one
with the highest personality score.

This means:

- Jaishi will never blunder for style.
- Jaishi will never refuse a winning tactic to preserve structure.
- When two moves are practically equal, Jaishi picks the more Jaishi-flavored one.

Disable at any time with `setoption name Personality value false` or set
`PersonalityStrength` to `0`.
