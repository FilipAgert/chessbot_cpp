#include "search/searcher.h"
#include "core/integer_representation.h"
#include "eval/eval.h"
#include "search/TT.h"
#include "search/time_manager.h"
#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <vector>
// non member functions
constexpr int MATE_BOUND = Eval::MATE_SCORE - 256; // anything beyond this is a mate score

inline int score_to_tt(int score, int ply) {
    if (score > MATE_BOUND)
        return std::min(Eval::MATE_SCORE, score + ply);
    if (score < -MATE_BOUND)
        return std::max(-Eval::MATE_SCORE, score - ply);
    return score;
}
inline int score_from_tt(int score, int ply) {
    if (score > MATE_BOUND)
        return score - ply;
    if (score < -MATE_BOUND)
        return score + ply;
    return score;
}
void insert_killer(Move move, std::array<Move, Parameters::NUM_KILLER_MOVES> &killers) {
    if (killers[0] != move && killers[1] != move) {
        std::swap(killers[0], killers[1]);
        killers[0] = move;
    }
}
void Searcher::new_game() {
    reset_killers();
    move_history.reset_history();
}

SearchResult Searcher::search(const Board &root, const StateStack &state_stack_, const Limits &limits) {
    // reset
    board = root;               // copy
    state_stack = state_stack_; // copy
    while (!restore_info_stack.empty())
        restore_info_stack.pop();
    while (!move_stack.empty())
        move_stack.pop();

    reset_killers();              // TODO: Shift up by two.
    move_history.reset_history(); // maybe dont need to clear unless newgame.

    stats = SearchStatistics{};
    in_null = false;
    root_best = Move();
    must_finish_search = false;

    // search
    bool is_white = board.get_turn_color() == pieces::white;
    SearchResult result = is_white ? think_loop<true>(limits) : think_loop<false>(limits);

    // check if we should ponder
    std::vector<Move> pv = tt.get_pv(board, 2, state_stack);

    if (pv.size() > 1 && result.bestmove.has_value()) {
        Move ponder_move = pv.at(1);
        Move bm = result.bestmove.value();
        if (board.is_move_legal(bm)) {

            RestoreMoveInfo info = board.get_turn_color() ? board.do_move<true>(bm) : board.do_move<false>(bm);
            bool is_valid = board.is_move_legal(ponder_move);
            board.undo_move(info, bm);
            if (is_valid)
                result.pondermove = std::make_optional(ponder_move);
        }
    }
    return result;
}
// member methods
Searcher::Searcher(Eval::Eval &eval, TranspositionTable &tt, ZobroistHasher &hasher, TimeManager &time_manager, const std::atomic<bool> &stop,
                   std::function<void(InfoMsg &&)> on_info, SearchSettings settings)
    : stop_requested(stop), on_info(std::move(on_info)), eval(eval), tt(tt), hasher(hasher), time_manager(time_manager), settings(settings) {
    precompute_reduction();
}

template <bool is_white> SearchResult Searcher::think_loop(const Limits &limits) {
    SearchResult res;
    int num_moves = board.get_moves<full_search, is_white>(move_arr[0]);
    if (num_moves > 0)
        res.bestmove = std::make_optional(move_arr[0][0]);
    else {
        if (num_moves == 0) {
            InfoMsg new_msg;
            new_msg.stringmsg = true;
            if (board.king_checked<is_white>()) {
                std::string othercol = is_white ? "black" : "white";
                new_msg.string = "mate detected " + othercol + " has won the game.";
            } else {
                new_msg.string = "draw detected";
            }
            on_info(std::move(new_msg));
            return res;
        }
    }

    int max_depth = limits.depth.has_value() ? limits.depth.value() + 1 : MAX_PLY;
    int preveval = 0;

    for (int depth = 1; ((depth < max_depth) && time_manager.get_should_start_new_iteration(depth)); depth++) {
        stats.seldepth = 0;
        root_best = Move();

        must_finish_search = (depth == 1);
        auto [eval, nt] = aspiration_search<is_white>(preveval, depth);
        must_finish_search = false;
        if (std::abs(eval) > MATE_BOUND) {
            // start matesearch at dmate = depth - 2
            int mate_ply = Eval::MATE_SCORE - std::abs(eval);
            int alpha = eval;
            int beta = Eval::MATE_SCORE;
            Move prev_best = root_best;
            if (eval > 0) { // Only do a mate search at shallow depths... need to
                            // benchmark but without prunings its slower
                int max_dmate = mate_ply - 2;
                if (max_dmate > 0) {
                    time_manager.set_matesearch_on(false);
                    for (int d = 1; d <= max_dmate; d += 2) {
                        if (!time_manager.get_should_start_new_iteration(d)) {
                            time_manager.set_matesearch_off();
                            break;
                        }
                        auto [eval_ms, nt_ms] = alpha_beta<true, is_white, false, true, NodeType::PV>(d, 0, alpha, beta, 0);
                        if (eval_ms > eval && root_best.is_valid() && nt_ms != SearchNodeType::timeout) {
                            eval = eval_ms;
                            alpha = eval;
                            time_manager.set_matesearch_off();
                            break;
                        } else
                            root_best = prev_best;
                    }
                }
            } else {
                // losing mate: search at depth without pruning to try to find an escape
                // or delay.
                int dmate = std::max(mate_ply, depth);
                time_manager.set_matesearch_on(true);
                auto [eval_ms, nt_ms] = alpha_beta<true, is_white, false, true, NodeType::PV>(dmate, 0, alpha, beta, 0);
                if (eval_ms > eval && nt_ms != SearchNodeType::timeout)
                    eval = eval_ms;
                else
                    root_best = prev_best;
                time_manager.set_matesearch_off();
            }
        }
        if (root_best.is_valid() && board.is_move_legal(root_best)) {
            res.bestmove = std::make_optional(root_best);
            res.depth = depth;
            res.score = eval;
        }

        if (nt == SearchNodeType::timeout)
            break;
        // std::pair<int, uint8_t> result = alpha_beta<true, is_white>(depth, 0,
        // alpha, beta, 0);
        InfoMsg new_msg;
        new_msg.nodes = stats.nodes_evaluated + stats.nodes_quiesence_evaluated;
        new_msg.time = time_manager.get_time_elapsed();
        new_msg.depth = depth;
        new_msg.pv = tt.get_pv(board, depth, state_stack);
        new_msg.seldepth = stats.seldepth;
        new_msg.hashfill = tt.load_factor();
        new_msg.score = eval;
        new_msg.reduction_average = (float(stats.reductions_total) / float(stats.reductions_cnt));
        new_msg.reduction_research = (float(stats.research_cnt) / float(stats.reductions_cnt));
        new_msg.reduction_times = stats.reductions_cnt;
        on_info(std::move(new_msg));

        preveval = eval;
        std::optional<int> moves_to_mate = Eval::moves_to_mate(eval);
        if (moves_to_mate) {
            break;
        }
        // Don't bother searching more if we only have 1 legal move & if we did not
        // specify a depth.
        if (num_moves == 1 && (!limits.depth)) {
            break;
        }
    }
    // to not send bestmove early if we go to max depth or find mate while in
    // ponder.
    while (time_manager.is_pondering() && !should_abort()) {
        std::this_thread::sleep_for(1ms);
    }
    return res;
}

template <bool is_white> std::pair<int, SearchNodeType> Searcher::aspiration_search(int preveval, int depth) {
    int delta = Parameters::ASPIRATION_WINDOW;
    int alpha = -INF;
    int beta = INF;
    if (depth > 1 && settings.enable_aspiration) {
        alpha = std::max(-INF, preveval - delta);
        beta = std::min(INF, preveval + delta);
    }
    int fails = 0;
    while (true) {
        auto res = alpha_beta<true, is_white, false, false, NodeType::PV>(depth, 0, alpha, beta, 0);
        if (res.second == SearchNodeType::timeout)
            return res;
        int score = res.first;
        if (score > alpha && score < beta)
            return res;
        if (score <= alpha)
            alpha = (++fails > 2) ? -INF : std::max(-INF, score - delta);
        else
            beta = (++fails > 2) ? INF : std::min(INF, score + delta);
        delta *= 4;
    }
    return {0, SearchNodeType::timeout};
}

template <bool is_root, bool is_white, bool is_zw, bool is_mate_search, NodeType node_type>
std::pair<int, SearchNodeType> Searcher::alpha_beta(int depth, int ply, int alpha, int beta, int num_extensions) {
    stats.nodes_evaluated++;
    stats.seldepth = std::max(ply, stats.seldepth);
    if (should_abort()) {
        return {0, SearchNodeType::timeout};
    }
    if (is_draw(ply)) {
        return {0, SearchNodeType::exact}; // Checks if position is a repeat.
    }
    if (depth <= 0) {
        if constexpr (!is_mate_search) {
            stats.nodes_evaluated--;
            return quiesence<is_white, false>(ply, alpha, beta);
        } else {
            if (alpha >= MATE_BOUND || beta < -MATE_BOUND) {
                // winning mate search
                if (board.king_checked<is_white>()) {
                    int num_moves = board.get_moves<full_search, is_white>(move_arr[ply]);
                    stats.moves_generated += num_moves;
                    if (num_moves == 0) {
                        if (board.king_checked<is_white>()) {
                            return {-Eval::MATE_SCORE + ply, SearchNodeType::exact};
                        } else {
                            return {0, SearchNodeType::exact};
                        }
                    }
                }
                return {eval.evaluate(board), SearchNodeType::exact};
            }
            stats.nodes_evaluated--;
            return quiesence<is_white, true>(ply, alpha, beta);
            // losing mate search enteres quiesence!
        }
    }

    if (ply >= MAX_PLY - 1) {
        return {eval.evaluate(board), SearchNodeType::exact};
    };

    Move best_curr_move;
    int bestscore = -INF;
    SearchNodeType nodetype = SearchNodeType::ub;

    uint64_t zob_hash = state_stack.top().hash;
    TTProbeResult tt_probe = probe_tt < !is_root && !is_mate_search && node_type != NodeType::PV > (zob_hash, depth, ply, alpha, beta);
    if (tt_probe.cutoff)
        return {tt_probe.score, tt_probe.bound};
    std::optional<Move> first_move = tt_probe.tt_move;

    bool king_checked = board.king_checked<is_white>();
    bool may_zugzwang = Eval::can_zugzwang<is_white>(board);
    int standing_pat = eval.evaluate(board);
    // reverse futilty pruning
    // conditions: not PV
    //             not in check
    //             tt move is not capture and exists.
    if constexpr (node_type != NodeType::PV) {
        if (first_move && !king_checked && !board.is_move_capture(first_move.value())) {
            int rfp;
            const int rfp_lim = beta + Parameters::RFP_DEPTH_SCALE * depth;
            if (tt_probe.bound == SearchNodeType::exact || tt_probe.bound == SearchNodeType::ub) {
                standing_pat = tt_probe.score;
                rfp = standing_pat;
            } else if (tt_probe.bound == SearchNodeType::lb && tt_probe.score > rfp_lim) {
                rfp = tt_probe.score;
            } else {
                rfp = standing_pat;
            }
            if (rfp > rfp_lim) {
                return {beta, SearchNodeType::lb};
            }
        }
    }

    // null move pruning
    // If we don't make a move, and the store is still beating beta, we are sure
    // this branch is not worth searching (Since our score is way too high) Even
    // doing nothing (worst possible move) is good.
    if constexpr (node_type != NodeType::CUT && !is_mate_search) {
        const int reduction = Parameters::NULLMOVE_R;
        if (!(king_checked | may_zugzwang) && (depth > reduction + 1) && !in_null) {
            in_null = true;
            make_nullmove<is_white>();
            std::pair<int, SearchNodeType> res =
                alpha_beta<false, !is_white, true, is_mate_search, NodeType::CUT>(depth - reduction - 1, ply + 1, -beta, -(beta - 1), num_extensions);
            int eval = -res.first;
            undo_move<is_white>();
            in_null = false;
            if (res.second == SearchNodeType::timeout)
                return res;
            if (eval >= beta) { // even doing nothing is good: prune
                return {beta, SearchNodeType::lb};
            }
        }
    }
    // Handle if king is checked or no moves can be made.

    int num_moves = board.get_moves<full_search, is_white>(move_arr[ply]);
    stats.moves_generated += num_moves;
    if (num_moves == 0) {
        if (king_checked) {
            return {-Eval::MATE_SCORE + ply, SearchNodeType::exact};
        } else {
            return {0, SearchNodeType::exact};
        }
    }
    std::array<Move, max_legal_moves> &moves = move_arr[ply];
    std::array<MoveOrder::MoveScore, max_legal_moves> &scores = move_heuristic_score[ply];
    std::array<Move, Parameters::NUM_KILLER_MOVES> &killers = killer_moves[ply];
    MoveOrder::move_heuristics<is_white>(eval.get_params().shared.piecevals, moves, scores, num_moves, board, first_move, move_history, killers);
    MoveOrder::MovePicker picker(moves, scores, num_moves);

    // Multicut pruning.
    // Applies in cut nodes with zero width.
    // Should benchmark later, apparently may be negative impact in some cases.
    // Here, it was 17 +/- 13 elo.
    if constexpr (is_zw && node_type == NodeType::CUT && !is_mate_search) {
        if (depth >= Parameters::MC_R && num_moves >= Parameters::MC_M) {
            int c = 0;
            int i = 0;
            while (picker.has_next() && i < Parameters::MC_M && (Parameters::MC_M - i) >= (Parameters::MC_C - c)) {
                i++;
                auto [_, move] = picker.next();

                make_move<is_white>(move);
                std::pair<int, SearchNodeType> res = alpha_beta<false, !is_white, is_zw, is_mate_search, NodeType::ALL>(depth - 1 - Parameters::MC_R, ply + 1,
                                                                                                                        -beta, -(beta - 1), num_extensions);
                undo_move<is_white>();
                if (res.second == SearchNodeType::timeout)
                    return res;

                if (-res.first >= beta) {
                    if (++c >= Parameters::MC_C) {
                        return {beta, SearchNodeType::lb};
                    }
                }
            }
            picker.reset_head(); // redo search for real.
        }
    }

    // Normal move generation.
    //
    while (picker.has_next()) {
        const auto [i, move] = picker.next();
        const MoveOrder::MoveScore &score = scores[i];

        bool move_quiet = board.is_move_quiet(move);
        make_move<is_white>(move);
        bool enemy_king_checked = board.king_checked<!is_white>();

        int extension = 0;
        if constexpr (!is_mate_search) {
            extension = calculate_extension(move, i, num_extensions, enemy_king_checked);
        } else if (alpha < MATE_BOUND && beta >= -MATE_BOUND) {
            // this branch if we are in a losing mate search.
            // a winning mate search gets no extensions.
            extension = calculate_extension(move, i, num_extensions, enemy_king_checked);
        }
        int eval;
        std::pair<int, SearchNodeType> res;
        if (i == 0) {
            constexpr NodeType child_node = node_type == NodeType::PV ? NodeType::PV : (node_type == NodeType::CUT ? NodeType::ALL : NodeType::CUT);
            res = alpha_beta<false, !is_white, is_zw, is_mate_search, child_node>(depth - 1 + extension, ply + 1, -beta, -alpha, num_extensions + extension);
        } else {
            // futility pruning:
            //  if move is quiet, and standing pat + margin*depth < alpha,
            //  if move does not check, it will likely not raise alpha.
            if constexpr (node_type != NodeType::PV) {
                uint8_t mt = score.movetype();
                int margin = Parameters::FUTILITY_PRUNING_MARGIN;
                if (depth <= 4 && !king_checked && !enemy_king_checked && (standing_pat + margin * depth <= alpha) &&
                    ((mt & (MoveOrder::MoveScore::QUIET_SAFE | MoveOrder::MoveScore::QUIET_UNSAFE)) > 0)) {
                    undo_move<is_white>();
                    continue;
                }
            }

            constexpr NodeType child_node = node_type == NodeType::CUT ? NodeType::ALL : NodeType::CUT;
            // PVS: Run null window search.
            int r = 0;
            // late move reduction:
            if constexpr (!is_root && !is_mate_search) {
                if (depth >= 3 && i >= 3 && move_quiet && (!king_checked) && (!enemy_king_checked) && extension == 0 &&
                    (score.movetype() != MoveOrder::MoveScore::KILLER)) {
                    r = reductions[std::min(depth, REDUCTION_DEPTH_CAP - 1)][i];
                    r = std::min(depth + extension - 2,
                                 r); // clamp to assure we do not go into quiesence.
                    stats.reductions_total += r;
                    stats.reductions_cnt++;
                }
            }
            res = alpha_beta<false, !is_white, true, is_mate_search, child_node>(depth - 1 + extension - r, ply + 1, -(alpha + 1), -alpha,
                                                                                 num_extensions + extension);
            if (res.second == SearchNodeType::timeout) {
                undo_move<is_white>();
                return res;
            }
            eval = -res.first;
            // full search, null window
            if (eval > alpha && r > 0) {
                res = alpha_beta<false, !is_white, true, is_mate_search, child_node>(depth - 1 + extension, ply + 1, -(alpha + 1), -alpha,
                                                                                     num_extensions + extension);
                if (res.second == SearchNodeType::timeout) {
                    undo_move<is_white>();
                    return res;
                }
                eval = -res.first;
                stats.research_cnt++;
            }

            // pvs kicks in: Full window. //should be nodetype PV here?
            if ((eval > alpha) && (eval < beta)) {
                res = alpha_beta<false, !is_white, is_zw, is_mate_search, NodeType::PV>(depth - 1 + extension, ply + 1, -beta, -alpha,
                                                                                        num_extensions + extension);
                if (res.second == SearchNodeType::timeout) {
                    undo_move<is_white>();
                    return res;
                }
            }
        }
        eval = -res.first;
        undo_move<is_white>();
        if (res.second == SearchNodeType::timeout)
            return res;
        if (eval > bestscore) {
            bestscore = eval;
            best_curr_move = move;
        }
        if constexpr (is_root) {
            if (eval > alpha)
                root_best = move;
        }
        if (eval >= beta) { // FAIL HIGH.
            tt.store(zob_hash, best_curr_move, score_to_tt(beta, ply), SearchNodeType::lb,
                     depth); // Can update hash to curr depth.

            if (move_quiet)
                insert_killer(move, killers);
            if (!board.is_move_capture(move)) {
                int bonus = depth * depth;
                move_history.update<is_white>(board.get_piece_at(move.source).get_type(), move.target, bonus);
                for (int j = static_cast<int>(i) - 1; j >= 0; j--) {
                    if (!board.is_move_capture(moves[j])) {
                        // penalize old moves.
                        move_history.update<is_white>(board.get_piece_at(moves[j].source).get_type(), moves[j].target, -bonus);
                    }
                }
            }
            return {beta, SearchNodeType::lb}; // This move is too good. The
                                               // minimising player (beta) will never

            // allow the board to go here. we can return.
        }
        if (eval > alpha) { // if alpha is raised, then we have found an exact node.
            alpha = eval;   // if alpha is never raised, the value returned will be an
                            // upper bound.
            nodetype = SearchNodeType::exact;
        }
    }
    tt.store(zob_hash, nodetype == SearchNodeType::exact ? best_curr_move : Move(), score_to_tt(alpha, ply), nodetype, depth);
    return {alpha, nodetype};
}

template <bool is_white, bool is_mate_search> std::pair<int, SearchNodeType> Searcher::quiesence(int ply, int alpha, int beta) {
    stats.nodes_quiesence_evaluated++;
    if (should_abort()) {
        return {0, SearchNodeType::timeout};
    }
    if (is_draw(ply))
        return {0, SearchNodeType::exact};
    if (ply >= MAX_PLY - 1) {
        return {eval.evaluate(board), SearchNodeType::exact};
    }

    uint64_t zob_hash = state_stack.top().hash;
    TTProbeResult probe = probe_tt<!is_mate_search>(zob_hash, 0, ply, alpha, beta);
    if (probe.cutoff)
        return {probe.score, probe.bound};
    std::optional<Move> tt_move = probe.tt_move;

    const bool in_check = board.king_checked<is_white>();
    const bool may_zugwang = Eval::can_zugzwang<is_white>(board);
    int standing_pat = eval.evaluate(board);
    if (!in_check) {
        if (!may_zugwang && standing_pat >= beta) {
            return {beta, SearchNodeType::lb};
        }

        // delta futility pruning
        // prune branch if we cannot improve score even with best capture and
        // promotion.
        if (eval.delta_futility_pruning(standing_pat, alpha, Parameters::DELTA_THRESHHOLD, board)) {
            return {alpha, SearchNodeType::ub};
        }
        alpha = std::max(alpha, standing_pat);
    }

    int num_moves = in_check ? board.get_moves<full_search, is_white>(move_arr[ply]) : board.get_moves<quiesence_search, is_white>(move_arr[ply]);
    stats.moves_generated += num_moves;
    if (num_moves == 0 && in_check)
        return {-Eval::MATE_SCORE + ply, SearchNodeType::exact};

    std::array<Move, max_legal_moves> &moves = move_arr[ply];
    std::array<MoveOrder::MoveScore, max_legal_moves> &scores = move_heuristic_score[ply];
    std::array<Move, Parameters::NUM_KILLER_MOVES> &killers = killer_moves[ply];
    MoveOrder::move_heuristics<is_white>(eval.get_params().shared.piecevals, moves, scores, num_moves, board, tt_move, move_history, killers);
    MoveOrder::MovePicker picker(moves, scores, num_moves);
    while (picker.has_next()) {
        const auto [i, move] = picker.next();
        const MoveOrder::MoveScore score = scores[i];
        if (!in_check && score.movetype() <= MoveOrder::MoveScore::SEE_BAD)
            break; // do not consider bad captures.
        // delta pruning:
        //  check if delta threshhold is reached
        if ((!in_check) && !(may_zugwang)) {
            int extra = eval.get_params().shared.piecevals[board.get_piece_at(move.target).get_type()];
            extra += eval.get_params().shared.piecevals[move.get_promotion()]; // doesn't detract pawn
                                                                               // value if promotion...
            extra += move.flag == moveflag::MOVEFLAG_pawn_ep_capture ? eval.get_params().shared.piecevals[pieces::pawn] : 0;
            assert(extra > 0);

            // worth checking branch!
            if (!(standing_pat + extra + Parameters::DELTA_THRESHHOLD > alpha))
                continue;
        }
        // end delta pruning

        make_move<is_white>(move);
        std::pair<int, SearchNodeType> res = quiesence<!is_white, is_mate_search>(ply + 1, -beta, -alpha);
        int eval = -res.first;
        undo_move<is_white>();
        if (res.second == SearchNodeType::timeout)
            return {0, SearchNodeType::timeout};
        if (eval >= beta) {
            if (board.is_move_quiet(move))
                insert_killer(move, killers);
            return {beta, SearchNodeType::lb};
        }
        alpha = std::max(eval, alpha);
    }

    return {alpha, SearchNodeType::ub};
}
template <bool is_white> void Searcher::make_nullmove() {
    Move nullmove = Move(0, 0, moveflag::MOVEFLAG_null_move);
    RestoreMoveInfo info = board.do_move<is_white, pieces::none, moveflag::MOVEFLAG_null_move, pieces::none>(nullmove);
    uint64_t hash = state_stack.top().hash ^ hasher.delta(info.delta);
    tt.prefetch(hash);
    move_stack.push(nullmove);
    restore_info_stack.push(info);
    state_stack.push({hash, board.get_num_pieces()}); // same number of pieces due to nullmove
    assert(state_stack.top() == hasher.hash_board(board));
}
template <bool is_white> void Searcher::make_move(Move move) {
    RestoreMoveInfo info = board.do_move<is_white>(move);
    uint64_t hash = state_stack.top().hash ^ hasher.delta(info.delta);
    tt.prefetch(hash);
    move_stack.push(move);
    restore_info_stack.push(info);
    state_stack.push({hash, board.get_num_pieces()});
    assert(state_stack.top().hash == hasher.hash_board(board));
}

template <bool is_white> void Searcher::undo_move() {
    Move move = move_stack.top();
    RestoreMoveInfo info = restore_info_stack.top();
    move_stack.pop();
    restore_info_stack.pop();
    board.undo_move<is_white>(info, move);
    state_stack.pop();
}

bool Searcher::should_abort() const { return stop_requested || (!must_finish_search && time_manager.get_should_stop()); }

int Searcher::reduction(int depth, int i) const {
    int r = std::floor(Parameters::REDUCTION_OFFSET + std::log(double(depth)) * std::log(double(i)) * Parameters::REDUCTION_MULT);
    return std::min(r, depth - 2);
}

void Searcher::precompute_reduction() {
    for (size_t depth = 1; depth < REDUCTION_DEPTH_CAP; depth++) {
        for (size_t i = 1; i < max_legal_moves; i++) {
            reductions[depth][i] = reduction(depth, i);
        }
    }
}
bool Searcher::is_repetition(int ply) {
    // Checks if we have repeated this board state.
    return state_stack.is_repetition(state_stack.len() - ply - 1);
}
bool Searcher::is_draw(int ply) { return is_repetition(ply) || Eval::forced_draw_ply(board) || Eval::insufficient_mating_material(board); }

int Searcher::calculate_extension(const Move move, uint8_t movenum, int num_extensions, bool enemy_king_checked) const {
    constexpr int max_num_extensions = 16;

    int extension = 0;
    if (num_extensions < max_num_extensions) {
        if (enemy_king_checked)
            extension = 1;
    }
    // if (movenum > 3)
    //     extension -= 1;
    return extension;
}

void Searcher::reset_killers() {
    static constexpr std::array<Move, Parameters::NUM_KILLER_MOVES> empty = []() {
        std::array<Move, Parameters::NUM_KILLER_MOVES> mp;
        for (size_t i = 0; i < Parameters::NUM_KILLER_MOVES; i++) {
            mp[i] = Move();
        }
        return mp;
    }();
    killer_moves.fill(empty);
}

void Searcher::ponderhit() { time_manager.ponderhit(); }

template <bool can_cutoff> TTProbeResult Searcher::probe_tt(uint64_t hash, int depth, int ply, int &alpha, int &beta) {
    TTProbeResult res;
    std::optional<TranspositionEntry> maybe_entry = tt.get(hash);
    if (maybe_entry) {
        const TranspositionEntry &entry = maybe_entry.value();
        if (entry.is_valid_move())
            res.tt_move = entry.bestmove;
        res.score = entry.eval;
        res.bound = entry.nodetype;

        if constexpr (can_cutoff) {
            if (tt.is_useable_entry(entry, depth)) {
                // If exact, return score
                int eval = score_from_tt(entry.eval, ply);
                bool is_mate = std::abs(eval) > MATE_BOUND;
                if (!is_mate || entry.age == tt.age) {
                    if (entry.is_exact()) { // if exact value
                        res.cutoff = true;
                        res.score = eval;
                    } else if (entry.is_lb()) {
                        if (eval >= beta) { // if its a lower bound, but this lower bound is
                                            // BETTER than
                            // any move opponent can make
                            // a lower bound can tighten alpha
                            res.score = beta;
                            res.cutoff = true;
                        }
                        alpha = std::max(alpha, eval);
                    } else if (entry.is_ub()) {
                        if (eval < alpha) { // if its an upper bound, and this upper bound is worse
                                            //
                            res.cutoff = true;
                            res.score = alpha;
                            // than any move we could make, dont need to search more.
                        }
                        beta = std::min(beta, eval);
                    }
                }
            }
        }
    }
    return res;
}
