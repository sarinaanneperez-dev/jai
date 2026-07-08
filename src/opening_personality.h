#pragma once
#include "constants.h"
#include "move.h"

namespace jaishi {

class Board;

// The OpeningPersonality module scores candidate moves on a set of stylistic
// axes. The final score is added to the search evaluation only when two
// candidate moves differ by less than `strength` centipawns — guaranteeing
// Jaishi never plays an objectively worse move for style, it only breaks ties.
class OpeningPersonality {
public:
    bool enabled = true;
    int  strength = 25; // centipawns; matches PersonalityStrength UCI option

    // Returns a stylistic score in centipawns for playing `m` from position `b`
    // (with the mover being `b.side_to_move`).
    int score_move(const Board& b, Move m) const;
};

extern OpeningPersonality PERSONALITY;

} // namespace jaishi
