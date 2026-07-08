#include "search.h"
#include "board.h"
#include "movegen.h"
#include "evaluation.h"
#include "tt.h"
#include "opening_personality.h"
#include <algorithm>
#include <iostream>
#include <cstring>
#include <cmath>

namespace jaishi {

SearchStats LAST_SEARCH;

namespace {

HistoryTable HISTORY;
TimeManager  TIMER;

// Search context
struct Stack {
    Move pv[MAX_PLY + 1];
    int  pv_len = 0;
    Move current_move = NULL_MOVE;
    int  static_eval = 0;
};

int  LMR_TABLE[MAX_PLY][MAX_MOVES];

void init_lmr() {
    for (int d = 0; d < MAX_PLY; ++d)
        for (int m = 0; m < MAX_MOVES; ++m) {
            if (d == 0 || m == 0) { LMR_TABLE[d][m] = 0; continue; }
            LMR_TABLE[d][m] = static_cast<int>(0.75 + std::log(double(d)) * std::log(double(m)) / 2.25);
        }
}
bool LMR_INIT_DONE = (init_lmr(), true);

// -------------- SEE helpers (internal) --------------
static int pt_val(PieceType pt) { return SEE_PIECE_VALUE[pt]; }

static U64 least_valuable_attacker(const Board& b, U64 attackers, Color c, PieceType& out_pt) {
    for (int pt = PAWN; pt <= KING; ++pt) {
        U64 bb = attackers & b.piece_bb[make_piece(c, static_cast<PieceType>(pt))];
        if (bb) {
            out_pt = static_cast<PieceType>(pt);
            return bb & -bb; // lowest bit
        }
    }
    return 0;
}

} // namespace

// -------------- SEE (public) --------------
int see_capture(Board& b, Move m) {
    int from = move_from(m);
    int to   = move_to(m);
    Piece attacker = b.piece_on(from);
    if (attacker == NO_PIECE) return 0;
    Piece victim = b.piece_on(to);
    int flag = move_flag(m);

    int gain[32];
    int d = 0;
    int captured_val = (victim == NO_PIECE) ? 0 : pt_val(type_of(victim));
    if (flag == FLAG_EP) captured_val = pt_val(PAWN);
    gain[d] = captured_val;

    PieceType aPt = type_of(attacker);
    U64 occ = b.all_bb ^ bit(from);
    if (flag == FLAG_EP) {
        int capSq = (color_of(attacker) == WHITE) ? to - 8 : to + 8;
        occ ^= bit(capSq);
    }
    U64 attackers = b.attackers_to(to, occ) & occ;
    Color stm = static_cast<Color>(color_of(attacker) ^ 1);

    while (true) {
        d++;
        gain[d] = pt_val(aPt) - gain[d - 1];
        if (std::max(-gain[d - 1], gain[d]) < 0) break;
        PieceType nextPt;
        U64 next = least_valuable_attacker(b, attackers, stm, nextPt);
        if (!next) break;
        attackers ^= next;
        occ       ^= next;
        // Update sliding attackers through this square
        U64 bq = (b.piece_bb[W_BISHOP] | b.piece_bb[B_BISHOP] | b.piece_bb[W_QUEEN] | b.piece_bb[B_QUEEN]) & occ;
        U64 rq = (b.piece_bb[W_ROOK]   | b.piece_bb[B_ROOK]   | b.piece_bb[W_QUEEN] | b.piece_bb[B_QUEEN]) & occ;
        attackers |= bishop_attacks(to, occ) & bq;
        attackers |= rook_attacks(to, occ)   & rq;
        attackers &= occ;
        aPt = nextPt;
        stm = static_cast<Color>(stm ^ 1);
        if (aPt == KING && (attackers & b.color_bb[stm])) break;
    }
    while (--d) gain[d - 1] = -std::max(-gain[d - 1], gain[d]);
    return gain[0];
}

namespace {

// -------------- Move ordering --------------
static int score_move(Board& b, Move m, Move tt_move, int ply) {
    if (m == tt_move) return 1'000'000;
    int flag = move_flag(m);
    int to   = move_to(m);
    Piece victim = b.piece_on(to);
    Piece mover  = b.piece_on(move_from(m));

    if (victim != NO_PIECE || flag == FLAG_EP) {
        int vv = (flag == FLAG_EP) ? PAWN : type_of(victim);
        int mv = mover != NO_PIECE ? type_of(mover) : PAWN;
        int mvv_lva = 100'000 + vv * 100 - mv;
        return mvv_lva;
    }
    if (flag == FLAG_PROMO) return 90'000 + move_promo(m);

    if (m == HISTORY.killer(ply, 0)) return 80'000;
    if (m == HISTORY.killer(ply, 1)) return 79'000;

    int side = b.side_to_move;
    return HISTORY.get_history(side, move_from(m), to);
}

static void order_moves(Board& b, MoveList& list, Move tt_move, int ply) {
    for (int i = 0; i < list.size; ++i)
        list.moves[i].score = score_move(b, list.moves[i].move, tt_move, ply);
}

static void pick_move(MoveList& list, int start) {
    int best = start;
    for (int i = start + 1; i < list.size; ++i)
        if (list.moves[i].score > list.moves[best].score) best = i;
    if (best != start) std::swap(list.moves[start], list.moves[best]);
}

// -------------- Quiescence --------------
int qsearch(Board& b, int alpha, int beta, int ply, Stack* ss) {
    LAST_SEARCH.nodes++;
    if ((LAST_SEARCH.nodes & 2047) == 0 && TIMER.should_stop()) return 0;
    if (ply >= MAX_PLY) return evaluate(b);

    int stand_pat = evaluate(b);
    if (stand_pat >= beta) return stand_pat;
    if (stand_pat > alpha) alpha = stand_pat;
    int best = stand_pat;

    MoveList list;
    generate_captures(b, list);
    order_moves(b, list, NULL_MOVE, ply);

    for (int i = 0; i < list.size; ++i) {
        pick_move(list, i);
        Move m = list.moves[i].move;

        // Delta / SEE pruning: skip clearly-losing captures
        int see_val = see_capture(b, m);
        if (see_val < 0) continue;

        if (!b.make_move(m)) continue;
        ss[ply].current_move = m;
        int score = -qsearch(b, -beta, -alpha, ply + 1, ss);
        b.undo_move();
        if (TIMER.should_stop()) return 0;

        if (score > best) {
            best = score;
            if (score > alpha) alpha = score;
            if (alpha >= beta) break;
        }
    }
    return best;
}

// -------------- Main search --------------
int negamax(Board& b, int depth, int alpha, int beta, int ply, bool null_ok, Stack* ss) {
    LAST_SEARCH.nodes++;
    if ((LAST_SEARCH.nodes & 2047) == 0 && TIMER.should_stop()) return 0;
    ss[ply].pv_len = 0;

    bool root = (ply == 0);
    bool in_check = b.in_check();

    if (!root) {
        if (b.is_repetition() || b.is_fifty_move_draw() || b.insufficient_material())
            return DRAW_SCORE;
        if (ply >= MAX_PLY) return evaluate(b);
        // Mate distance pruning
        alpha = std::max(alpha, -MATE_SCORE + ply);
        beta  = std::min(beta,   MATE_SCORE - ply - 1);
        if (alpha >= beta) return alpha;
    }

    // Check extension
    if (in_check) depth++;

    if (depth <= 0) return qsearch(b, alpha, beta, ply, ss);

    // TT probe
    TTEntry tte;
    Move tt_move = NULL_MOVE;
    bool tt_hit = TT.probe(b.zobrist_key, tte);
    if (tt_hit) {
        tt_move = tte.best;
        if (!root && tte.depth >= depth) {
            int s = tte.score;
            if (s >=  MATE_IN_MAX) s -= ply;
            if (s <= -MATE_IN_MAX) s += ply;
            if (tte.flag == TT_EXACT) return s;
            if (tte.flag == TT_LOWER && s >= beta) return s;
            if (tte.flag == TT_UPPER && s <= alpha) return s;
        }
    }

    int static_eval = in_check ? -INF_SCORE : evaluate(b);
    ss[ply].static_eval = static_eval;

    // Reverse futility / static null
    if (!in_check && !root && depth <= 6 && static_eval - 85 * depth >= beta)
        return static_eval;

    // Null-move pruning
    if (null_ok && !in_check && !root && depth >= 3 && static_eval >= beta) {
        // Only if we have non-pawn material
        Color us = b.side_to_move;
        U64 non_pawn = b.color_bb[us]
            & ~(b.piece_bb[make_piece(us, PAWN)] | b.piece_bb[make_piece(us, KING)]);
        if (non_pawn) {
            int R = 3 + depth / 6;
            b.make_null_move();
            int score = -negamax(b, depth - 1 - R, -beta, -beta + 1, ply + 1, false, ss);
            b.undo_null_move();
            if (TIMER.should_stop()) return 0;
            if (score >= beta) {
                if (score >= MATE_IN_MAX) score = beta;
                return score;
            }
        }
    }

    MoveList list;
    generate_all_moves(b, list);
    order_moves(b, list, tt_move, ply);

    int best = -INF_SCORE;
    Move best_move = NULL_MOVE;
    int moves_played = 0;
    int original_alpha = alpha;

    for (int i = 0; i < list.size; ++i) {
        pick_move(list, i);
        Move m = list.moves[i].move;

        if (!b.make_move(m)) continue;
        moves_played++;
        ss[ply].current_move = m;

        bool is_capture = (move_flag(m) == FLAG_EP) || (b.undo_stack[b.undo_size - 1].captured != NO_PIECE);
        bool is_promo   = (move_flag(m) == FLAG_PROMO);
        bool gives_check = b.in_check();

        int new_depth = depth - 1;
        int score;

        // Late move reduction
        int reduction = 0;
        if (depth >= 3 && moves_played > 3 && !in_check && !is_capture && !is_promo && !gives_check) {
            reduction = LMR_TABLE[std::min(depth, MAX_PLY - 1)][std::min(moves_played, MAX_MOVES - 1)];
            if (reduction < 0) reduction = 0;
            if (reduction >= new_depth) reduction = new_depth - 1;
            if (reduction < 0) reduction = 0;
        }

        if (moves_played == 1) {
            score = -negamax(b, new_depth, -beta, -alpha, ply + 1, true, ss);
        } else {
            score = -negamax(b, new_depth - reduction, -alpha - 1, -alpha, ply + 1, true, ss);
            if (score > alpha && reduction > 0)
                score = -negamax(b, new_depth, -alpha - 1, -alpha, ply + 1, true, ss);
            if (score > alpha && score < beta)
                score = -negamax(b, new_depth, -beta, -alpha, ply + 1, true, ss);
        }

        b.undo_move();
        if (TIMER.should_stop()) return 0;

        if (score > best) {
            best = score;
            best_move = m;
            if (score > alpha) {
                alpha = score;
                // update PV
                ss[ply].pv[0] = m;
                int cl = ss[ply + 1].pv_len;
                for (int j = 0; j < cl; ++j)
                    ss[ply].pv[j + 1] = ss[ply + 1].pv[j];
                ss[ply].pv_len = cl + 1;

                if (alpha >= beta) {
                    if (!is_capture) {
                        HISTORY.update_killer(ply, m);
                        HISTORY.update_history(b.side_to_move, move_from(m), move_to(m), depth);
                    }
                    break;
                }
            }
        }
    }

    if (moves_played == 0) {
        // Checkmate or stalemate
        return in_check ? -MATE_SCORE + ply : DRAW_SCORE;
    }

    TTFlag flag = (best >= beta) ? TT_LOWER
                : (best > original_alpha) ? TT_EXACT
                : TT_UPPER;
    TT.store(b.zobrist_key, best_move, best, depth, ply, flag);
    return best;
}

} // namespace

Move search_best_move(Board& board, const SearchLimits& limits) {
    STOP_FLAG.store(false);
    TIMER.start(limits, board.side_to_move);
    TT.new_search();
    HISTORY.clear();
    LAST_SEARCH = SearchStats{};

    Stack ss[MAX_PLY + 4];
    for (auto& s : ss) { s.pv_len = 0; s.current_move = NULL_MOVE; s.static_eval = 0; }

    Move best_move = NULL_MOVE;
    int  best_score = 0;
    int  max_depth = limits.depth > 0 ? limits.depth : MAX_PLY - 1;

    // Personality bias at the root: pre-sort root moves by (search score + style),
    // then use standard iterative deepening. We do that by adding personality to
    // history *only for near-equal alternatives*: the actual selection happens
    // after search below.
    int alpha = -INF_SCORE, beta = INF_SCORE;

    for (int depth = 1; depth <= max_depth; ++depth) {
        // Aspiration windows above depth 4
        int window = 30;
        if (depth >= 5) {
            alpha = std::max(-INF_SCORE, best_score - window);
            beta  = std::min( INF_SCORE, best_score + window);
        }

        int score;
        while (true) {
            score = negamax(board, depth, alpha, beta, 0, true, ss);
            if (TIMER.should_stop()) break;
            if (score <= alpha) { alpha = std::max(-INF_SCORE, alpha - window); window *= 2; }
            else if (score >= beta) { beta = std::min(INF_SCORE, beta + window); window *= 2; }
            else break;
        }

        if (TIMER.should_stop() && best_move != NULL_MOVE) break;

        best_score = score;
        if (ss[0].pv_len > 0) best_move = ss[0].pv[0];
        LAST_SEARCH.depth_reached = depth;
        LAST_SEARCH.score = best_score;
        LAST_SEARCH.best_move = best_move;
        LAST_SEARCH.time_ms = TIMER.elapsed_ms();

        // UCI info line
        std::cout << "info depth " << depth
                  << " score cp " << best_score
                  << " nodes " << LAST_SEARCH.nodes
                  << " time " << LAST_SEARCH.time_ms
                  << " pv";
        for (int i = 0; i < ss[0].pv_len; ++i)
            std::cout << " " << move_to_uci(ss[0].pv[i]);
        std::cout << std::endl;

        // Time budget: if we've spent more than half the budget, stop deepening.
        if (limits.movetime_ms == 0 && !limits.infinite && limits.depth == 0) {
            long long budget = 0;
            if (board.side_to_move == WHITE) budget = limits.wtime;
            else                             budget = limits.btime;
            if (budget > 0 && LAST_SEARCH.time_ms * 2 > budget / 30) {
                // heuristic; TIMER.should_stop() covers hard limit
            }
        }
    }

    // Personality tie-break at the root — only when candidates are within
    // PersonalityStrength cp of the best.
    if (best_move != NULL_MOVE && PERSONALITY.enabled && PERSONALITY.strength > 0) {
        MoveList list;
        generate_all_moves(board, list);
        // Re-search each root move at reduced depth to compare against best_score
        int chosen_bonus = PERSONALITY.score_move(board, best_move);
        Move candidate = best_move;
        int  cand_bonus = chosen_bonus;
        // Only inspect the top few moves (already move-ordered)
        order_moves(board, list, best_move, 0);
        int limit = std::min(list.size, 8);
        for (int i = 0; i < limit; ++i) {
            pick_move(list, i);
            Move m = list.moves[i].move;
            if (m == best_move) continue;
            if (!board.make_move(m)) continue;
            int score = -negamax(board, std::max(1, LAST_SEARCH.depth_reached - 2),
                                 -INF_SCORE, INF_SCORE, 1, true, ss);
            board.undo_move();
            if (best_score - score > PERSONALITY.strength) continue;
            int style = PERSONALITY.score_move(board, m);
            if (style > cand_bonus) {
                candidate = m;
                cand_bonus = style;
            }
        }
        best_move = candidate;
        LAST_SEARCH.best_move = candidate;
    }

        // Debug: print FEN before and after playing the best move
    std::cout << "DEBUG: Before playing best move: " << board.to_fen() << std::endl;
    if (!board.make_move(best_move)) {
        std::cerr << "ERROR: Best move is illegal!\n";
        return NULL_MOVE;
    }
    std::cout << "DEBUG: After playing best move: " << board.to_fen() << std::endl;
    board.undo_move();

    return best_move;
}

} // namespace jaishi
