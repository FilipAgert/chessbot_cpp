// Copyright 2025 Filip Agert
#include "constants.h"
#include "integer_representation.h"
#include "movegen.h"
#include "piece.h"
#include "tables.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <moveorder.h>

#include "parameters.h"
#include <board.h>
#include <climits> // For infinity
#include <config.h>
#include <eval.h>
#include <game.h>
#include <memory>
#include <move.h>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>
#include <time_manager.h>
#include <utility>
bool Game::set_fen(std::string FEN) {
    bool success = board.read_fen(FEN);
    assert(board.board_BB_match());
    reset_state_stack();
    return success;
}

void Game::start_game() {
    reset_state_stack();
    trans_table.clear();
    trans_table.reset_age();
    reset_infos();
}

void Game::abort_search_and_wait() {
    if (search_thread_.joinable()) {
        stop_requested_ = true;
        search_thread_.join();
    }
    search_finished = true;
    stop_requested_ = false;
}
void Game::start_thinking(const TimeControl rem_time, std::optional<int> depth, bool assign_worker) {
    abort_search_and_wait();
    int buffer = STANDARD_TIME_BUFFER; // ms
    int fraction = STANDARD_TIME_FRAC; // spend 1/20th of remaining time.

    bool is_white = board.get_turn_color() == pieces::white;
    time_manager = std::make_shared<TimeManager>(rem_time, buffer, fraction, is_white, !depth.has_value());
    time_manager->start_time_management();
    auto run_search = [this, depth, is_white]() {
        reset_infos();
        if (is_white)
            think_loop<true>(depth);
        else
            think_loop<false>(depth);

        // check if we should ponder
        std::vector<Move> pv = trans_table.get_pv(board, 2, state_stack);
        if (pv.size() > 1) {
            Move ponder_move = pv.at(1);
            Move bm = get_bestmove();
            if (board.is_move_valid(bm)) {

                RestoreMoveInfo info = board.get_turn_color() ? board.do_move<true>(bm) : board.do_move<false>(bm);
                bool is_valid = board.is_move_valid(ponder_move);
                board.undo_move(info, bm);
                if (is_valid)
                    set_pondermove(ponder_move);
            }
        }

        time_manager->stop_and_join(); // Join time manager thread to this one.
        search_finished = true;
    };

    search_finished = false;
    if (assign_worker) {
        search_thread_ = std::jthread(std::move(run_search));
    } else {
        run_search();
    }
}
void Game::reset_infos() {
    moves_generated = 0;
    nodes_evaluated = 0;
    nodes_quiesence_evaluated = 0;
    reductions_cnt = 0;
    reductions_total = 0;
    research_cnt = 0;
    set_bestmove(Move());
    in_null = false;
    reset_killers();
    history.reset_history();
}

void Game::reset_killers() {
    static constexpr std::array<Move, NUM_KILLER_MOVES> empty = []() {
        std::array<Move, NUM_KILLER_MOVES> mp;
        for (size_t i = 0; i < NUM_KILLER_MOVES; i++) {
            mp[i] = Move();
        }
        return mp;
    }();
    killer_moves.fill(empty);
}
void insert_killer(Move move, std::array<Move, NUM_KILLER_MOVES> &killers) {
    if (killers[0] != move && killers[1] != move) {
        std::swap(killers[0], killers[1]);
        killers[0] = move;
    }
}

void Game::reset_state_stack() {
    state_stack.reset();
    uint64_t board_hash = ZobroistHasher::get().hash_board(board);
    state_stack.push({board_hash, board.get_num_pieces()});
}

constexpr int MATE_BOUND = EvalState::MATE_SCORE - 256; // anything beyond this is a mate score

inline int score_to_tt(int score, int ply) {
    if (score > MATE_BOUND)
        return std::min(EvalState::MATE_SCORE, score + ply);
    if (score < -MATE_BOUND)
        return std::max(-EvalState::MATE_SCORE, score - ply);
    return score;
}
inline int score_from_tt(int score, int ply) {
    if (score > MATE_BOUND)
        return score - ply;
    if (score < -MATE_BOUND)
        return score + ply;
    return score;
}

InfoMsg Game::info_queue_front() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    return info_queue.front();
}
void Game::info_queue_pop() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    info_queue.pop();
}
bool Game::info_queue_empty() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    return info_queue.empty();
}
void Game::push_msg(InfoMsg &&msg) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    info_queue.push(msg);
}

template <bool is_white> void Game::think_loop(std::optional<int> indepth) {
    trans_table.increment_age();
    trans_table.increment_age(); // they played a move also.
    int num_moves = board.get_moves<full_search, is_white>(move_arr[0]);
    if (num_moves > 0)
        set_bestmove(move_arr[0][0]);
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
            push_msg(std::move(new_msg));
            search_finished = true;
            return;
        }

        // if (num_moves == 1) {
        //     search_finished = true;
        //     return;
        // }
    }
    int max_depth = indepth.has_value() ? indepth.value() + 1 : 256;
    assert(board.board_BB_match());
    int preveval = 0;
    for (int depth = 1; depth < max_depth; depth++) {
        seldepth = 0;
        if (!time_manager->get_should_start_new_iteration())
            break;
        root_best = Move();
        must_finish = (depth == 1);
        auto [eval, nt] = aspiration_search<is_white>(preveval, depth);
        if (std::abs(eval) > MATE_BOUND) {
            // start matesearch at dmate = depth - 2
            int mate_ply = EvalState::MATE_SCORE - std::abs(eval);
            int alpha = eval;
            int beta = EvalState::MATE_SCORE;
            Move prev_best = root_best;
            if (eval > 0) { // Only do a mate search at shallow depths... need to
                            // benchmark but without prunings its slower
                int max_dmate = mate_ply - 2;
                if (max_dmate > 0) {
                    time_manager->set_matesearch_on(false);
                    for (int d = 1; d <= max_dmate; d += 2) {
                        if (!time_manager->get_should_start_new_iteration()) {
                            time_manager->set_matesearch_off();
                            break;
                        }
                        auto [eval_ms, nt_ms] = alpha_beta<true, is_white, false, true, NodeType::PV>(d, 0, alpha, beta, 0);
                        if (eval_ms > eval && root_best.is_valid() && nt_ms != SearchNodeType::timeout) {
                            eval = eval_ms;
                            alpha = eval;
                            time_manager->set_matesearch_off();
                            break;
                        } else
                            root_best = prev_best;
                    }
                }
            } else {
                // losing mate: search at depth without pruning to try to find an escape
                // or delay.
                int dmate = std::max(mate_ply, depth);
                time_manager->set_matesearch_on(true);
                auto [eval_ms, nt_ms] = alpha_beta<true, is_white, false, true, NodeType::PV>(dmate, 0, alpha, beta, 0);
                if (eval_ms > eval && nt_ms != SearchNodeType::timeout)
                    eval = eval_ms;
                else
                    root_best = prev_best;
                time_manager->set_matesearch_off();
            }
        }
        must_finish = false;
        if (root_best.is_valid())
            set_bestmove(root_best);

        if (nt == SearchNodeType::timeout)
            break;
        // std::pair<int, uint8_t> result = alpha_beta<true, is_white>(depth, 0,
        // alpha, beta, 0);
        InfoMsg new_msg;
        new_msg.nodes = nodes_evaluated + nodes_quiesence_evaluated;
        new_msg.time = time_manager->get_time_elapsed();
        new_msg.depth = depth;
        new_msg.pv = trans_table.get_pv(board, depth, state_stack);
        new_msg.seldepth = seldepth;
        new_msg.hashfill = trans_table.load_factor();
        new_msg.score = eval;
        new_msg.reduction_average = (float(reductions_total) / float(reductions_cnt));
        new_msg.reduction_research = (float(research_cnt) / float(reductions_cnt));
        new_msg.reduction_times = reductions_cnt;
        push_msg(std::move(new_msg));

        preveval = eval;
        std::optional<int> moves_to_mate = EvalState::moves_to_mate(eval);
        if (moves_to_mate) {
            break;
        }
        // Don't bother searching more if we only have 1 legal move & if we did not
        // specify a depth.
        if (num_moves == 1 && (!indepth)) {
            break;
        }
    }
    // to not send bestmove early if we go to max depth or find mate while in
    // ponder.
    while (time_manager->is_pondering() && !should_abort()) {
        std::this_thread::sleep_for(1ms);
    }
}

template <bool is_white> std::pair<int, SearchNodeType> Game::aspiration_search(int preveval, int depth) {
    int delta = Parameters::ASPIRATION_WINDOW;
    int alpha = -INF;
    int beta = INF;
    if (depth > 1 && settings_.aspiration_enable) {
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
        delta += delta / 2;
    }
}

template <bool is_root, bool is_white, bool is_zw, bool is_mate_search, uint8_t node_type>
std::pair<int, SearchNodeType> Game::alpha_beta(int depth, int ply, int alpha, int beta, int num_extensions) {
    nodes_evaluated++;
    seldepth = std::max(ply, seldepth);
    if (is_repetition(ply) || EvalState::forced_draw_ply(board)) {
        return {0, SearchNodeType::exact}; // Checks if position is a repeat.
    }
    if constexpr (!is_root) {
        if (EvalState::insufficient_mating_material(board))
            return {0, SearchNodeType::exact};
    }

    if (depth <= 0) {
        if constexpr (!is_mate_search) {
            nodes_evaluated--;
            return quiesence<is_white, false>(ply, alpha, beta);
        } else {
            if (alpha >= MATE_BOUND || beta < -MATE_BOUND) {
                // winning mate search
                if (board.king_checked<is_white>()) {
                    int num_moves = board.get_moves<full_search, is_white>(move_arr[ply]);
                    moves_generated += num_moves;
                    if (num_moves == 0) {
                        if (board.king_checked<is_white>()) {
                            return {-EvalState::MATE_SCORE + ply, SearchNodeType::exact};
                        } else {
                            return {0, SearchNodeType::exact};
                        }
                    }
                }
                return {eval.eval(board), SearchNodeType::exact};
            }
            nodes_evaluated--;
            return quiesence<is_white, true>(ply, alpha, beta);
            // losing mate search enteres quiesence!
        }
    }

    if (ply >= MAX_PLY - 1) {
        return {eval.eval(board), SearchNodeType::exact};
    };

    Move best_curr_move;
    int bestscore = -INF;
    SearchNodeType nodetype = SearchNodeType::ub;

    uint64_t zob_hash = state_stack.top().hash;
    std::optional<TranspositionEntry> maybe_entry = trans_table.get(zob_hash);
    std::optional<Move> first_move = {};
    if (maybe_entry) {
        const TranspositionEntry &entry = maybe_entry.value();
        if (entry.is_valid_move())
            first_move = std::make_optional(maybe_entry.value().bestmove);

        if constexpr (!is_root && !is_mate_search) {
            if (trans_table.is_useable_entry(entry, depth)) {
                // If exact, return score
                int eval = score_from_tt(entry.eval, ply);
                bool is_mate = std::abs(eval) > MATE_BOUND;
                if (!is_mate || entry.age == trans_table.age) {
                    if (entry.is_exact()) { // if exact value
                        return {eval, SearchNodeType::exact};
                    } else if (entry.is_lb()) {
                        if (eval >= beta) // if its a lower bound, but this lower bound is
                                          // BETTER than
                            // any move opponent can make
                            return {beta, SearchNodeType::lb};
                        else
                            alpha = std::max(alpha,
                                             eval); // a lower bound can still tighten alpha./
                    } else if (entry.is_ub()) {
                        if (eval < alpha) // if its an upper bound, and this upper bound is worse
                                          // than any move we could make, dont need to search more.
                            return {alpha, SearchNodeType::ub};
                        else
                            beta = std::min(beta,
                                            eval); // an upper bound can still tighten beta.
                    }
                }
            }
        }
    }
    bool king_checked = board.king_checked<is_white>();
    bool may_zugzwang = EvalState::can_zugzwang<is_white>(board);

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
            if (eval >= beta) { // even doing nothing is good: prune
                return {beta, SearchNodeType::lb};
            }
        }
    }
    // Handle if king is checked or no moves can be made.

    int num_moves = board.get_moves<full_search, is_white>(move_arr[ply]);
    moves_generated += num_moves;
    if (num_moves == 0) {
        if (king_checked) {
            return {-EvalState::MATE_SCORE + ply, SearchNodeType::exact};
        } else {
            return {0, SearchNodeType::exact};
        }
    }
    std::array<Move, max_legal_moves> &moves = move_arr[ply];
    std::array<MoveOrder::MoveScore, max_legal_moves> &scores = move_heuristic_score[ply];
    std::array<Move, NUM_KILLER_MOVES> &killers = killer_moves[ply];
    MoveOrder::move_heuristics<is_white>(eval.params.piecevals, moves, scores, num_moves, board, first_move, history, killers);
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

                if (-res.first >= beta) {
                    if (++c >= Parameters::MC_C) {
                        return {beta, SearchNodeType::ub};
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
            constexpr uint8_t child_type = node_type == NodeType::PV ? NodeType::PV : (node_type == NodeType::CUT ? NodeType::ALL : NodeType::CUT);
            res = alpha_beta<false, !is_white, is_zw, is_mate_search, child_type>(depth - 1 + extension, ply + 1, -beta, -alpha, num_extensions + extension);
        } else {
            constexpr uint8_t child_type = node_type == NodeType::CUT ? NodeType::ALL : NodeType::CUT;
            // PVS: Run null window search.
            int r = 0;
            // late move reduction:
            if constexpr (!is_root && !is_mate_search) {
                if (depth >= 3 && i >= 3 && move_quiet && (!king_checked) && (!enemy_king_checked) && extension == 0 &&
                    (score.movetype() != MoveOrder::MoveScore::KILLER)) {
                    r = reductions[depth][i];
                    r = std::min(depth + extension - 2,
                                 r); // clamp to assure we do not go into quiesence.
                    reductions_total += r;
                    reductions_cnt++;
                }
            }
            res = alpha_beta<false, !is_white, true, is_mate_search, child_type>(depth - 1 + extension - r, ply + 1, -(alpha + 1), -alpha,
                                                                                 num_extensions + extension);
            eval = -res.first;
            // full search, null window
            if (eval > alpha && r > 0) {
                res = alpha_beta<false, !is_white, true, is_mate_search, child_type>(depth - 1 + extension, ply + 1, -(alpha + 1), -alpha,
                                                                                     num_extensions + extension);
                eval = -res.first;
                research_cnt++;
            }

            // pvs kicks in: Full window. //should be nodetype PV here?
            if ((eval > alpha) && (eval < beta))
                res = alpha_beta<false, !is_white, is_zw, is_mate_search, NodeType::PV>(depth - 1 + extension, ply + 1, -beta, -alpha,
                                                                                        num_extensions + extension);
        }
        eval = -res.first;
        undo_move<is_white>();
        if (should_abort()) {
            return {0, SearchNodeType::timeout};
        }
        if (eval > bestscore) {
            bestscore = eval;
            best_curr_move = move;
        }
        if constexpr (is_root) {
            if (eval > alpha)
                root_best = move;
        }
        if (eval >= beta) { // FAIL HIGH.
            trans_table.store(zob_hash, best_curr_move, score_to_tt(beta, ply), SearchNodeType::lb,
                              depth); // Can update hash to curr depth.

            if (move_quiet)
                insert_killer(move, killers);
            if (!board.is_move_capture(move)) {
                int bonus = depth * depth;
                history.update<is_white>(board.get_piece_at(move.source).get_type(), move.target, bonus);
                for (int j = static_cast<int>(i) - 1; j >= 0; j--) {
                    if (!board.is_move_capture(moves[j])) {
                        // penalize old moves.
                        history.update<is_white>(board.get_piece_at(moves[j].source).get_type(), moves[j].target, -bonus);
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
    trans_table.store(zob_hash, nodetype == SearchNodeType::exact ? best_curr_move : Move(), score_to_tt(alpha, ply), nodetype, depth);
    return {alpha, nodetype};
}

template <bool is_white, bool is_mate_search> std::pair<int, SearchNodeType> Game::quiesence(int ply, int alpha, int beta) {
    nodes_quiesence_evaluated++;
    if (is_repetition(ply) || EvalState::forced_draw_ply(board))
        return {0, SearchNodeType::exact};
    if (EvalState::insufficient_mating_material(board))
        return {0, SearchNodeType::exact};
    if (ply >= MAX_PLY - 1) {
        return {eval.eval(board), SearchNodeType::exact};
    }

    uint64_t zob_hash = state_stack.top().hash;
    std::optional<TranspositionEntry> maybe_entry = trans_table.get(zob_hash);
    std::optional<Move> tt_move;
    if (maybe_entry) {
        const TranspositionEntry &entry = maybe_entry.value();
        // transposition table hits may be corrupted. disable them if in mate
        // search.
        if constexpr (!is_mate_search) {
            if (trans_table.is_useable_entry(entry, -10000)) { // depth -1000: any stored depth qualifies
                int tt_score = score_from_tt(entry.eval, ply);
                bool is_mate = abs(tt_score) > MATE_BOUND;
                if (!is_mate || trans_table.age == entry.age) {
                    if (entry.is_exact())
                        return {tt_score, SearchNodeType::exact};
                    if (entry.is_lb()) {
                        if (tt_score >= beta)
                            return {beta, SearchNodeType::lb};
                        alpha = std::max(alpha, tt_score);
                    }
                    if (entry.is_ub()) {
                        if (tt_score <= alpha)
                            return {alpha, SearchNodeType::ub};
                        beta = std::min(beta, tt_score);
                    }
                }
            }
        }
        if (entry.is_valid_move())
            tt_move = std::make_optional(entry.bestmove);
    }

    const bool in_check = board.king_checked<is_white>();
    const bool may_zugwang = EvalState::can_zugzwang<is_white>(board);
    int standing_pat = eval.eval(board);
    if (!in_check) {
        if (!may_zugwang && standing_pat >= beta) {
            return {beta, SearchNodeType::lb};
        }

        // delta futility pruning
        // prune branch if we cannot improve score even with best capture and
        // promotion.
        if (delta_futility_pruning<is_white>(standing_pat, alpha)) {
            return {alpha, SearchNodeType::ub};
        }
        alpha = std::max(alpha, standing_pat);
    }

    int num_moves = in_check ? board.get_moves<full_search, is_white>(move_arr[ply]) : board.get_moves<quiesence_search, is_white>(move_arr[ply]);
    moves_generated += num_moves;
    if (num_moves == 0 && in_check)
        return {-EvalState::MATE_SCORE + ply, SearchNodeType::exact};

    std::array<Move, max_legal_moves> &moves = move_arr[ply];
    std::array<MoveOrder::MoveScore, max_legal_moves> &scores = move_heuristic_score[ply];
    std::array<Move, NUM_KILLER_MOVES> &killers = killer_moves[ply];
    MoveOrder::move_heuristics<is_white>(eval.params.piecevals, moves, scores, num_moves, board, tt_move, history, killers);
    MoveOrder::MovePicker picker(moves, scores, num_moves);
    while (picker.has_next()) {
        const auto [i, move] = picker.next();
        const MoveOrder::MoveScore score = scores[i];
        if (!in_check && score.movetype() <= MoveOrder::MoveScore::SEE_BAD)
            break; // do not consider bad captures.
        // delta pruning:
        //  check if delta threshhold is reached
        if ((!in_check) && !(may_zugwang)) {
            int extra = eval.params.piecevals[board.get_piece_at(move.target).get_type()];
            extra += eval.params.piecevals[move.get_promotion()]; // doesn't detract pawn
                                                                  // value if promotion...
            extra += move.flag == moveflag::MOVEFLAG_pawn_ep_capture ? eval.params.piecevals[pieces::pawn] : 0;
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
        if (should_abort()) {
            return {0, SearchNodeType::timeout};
        }
        if (eval >= beta) {
            if (board.is_move_quiet(move))
                insert_killer(move, killers);
            return {beta, SearchNodeType::lb};
        }
        alpha = std::max(eval, alpha);
    }

    return {alpha, SearchNodeType::ub};
}

int Game::calculate_extension(const Move move, uint8_t movenum, int num_extensions, bool enemy_king_checked) const {
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

bool Game::is_repetition(int ply) {
    // Checks if we have repeated this board state.
    return state_stack.is_repetition(state_stack.len() - ply - 1);
}
Move Game::get_bestmove() const {
    std::lock_guard<std::mutex> lock(bestmove_mutex);
    return bestmove;
}
Move Game::get_pondermove() const {
    std::lock_guard<std::mutex> lock(pondermove_mutex);
    return pondermove;
}

void Game::set_bestmove(Move move) {
    std::lock_guard<std::mutex> lock(bestmove_mutex);
    bestmove = move;
}
void Game::set_pondermove(Move move) {
    std::lock_guard<std::mutex> lock(pondermove_mutex);
    pondermove = move;
}

std::string Game::get_fen() const { return board.fen_from_state(); }

template <bool is_white> void Game::make_nullmove() {
    Move nullmove = Move(0, 0, moveflag::MOVEFLAG_null_move);
    RestoreMoveInfo info = board.do_move<is_white, pieces::none, moveflag::MOVEFLAG_null_move, pieces::none>(nullmove);
    uint64_t hash = state_stack.top().hash ^ ZobroistHasher::get().delta(info.delta);
    trans_table.prefetch(hash);
    move_stack.push(nullmove);
    restore_info_stack.push(info);
    state_stack.push({hash, board.get_num_pieces()}); // same number of pieces due to nullmove
    assert(state_stack.top() == ZobroistHasher::get().hash_board(board));
}
template <bool is_white> void Game::make_move(Move move) {
    RestoreMoveInfo info = board.do_move<is_white>(move);
    uint64_t hash = state_stack.top().hash ^ ZobroistHasher::get().delta(info.delta);
    trans_table.prefetch(hash);
    move_stack.push(move);
    restore_info_stack.push(info);
    state_stack.push({hash, board.get_num_pieces()});
    assert(state_stack.top().hash == ZobroistHasher::get().hash_board(board));
}

template <bool is_white> void Game::undo_move() {
    Move move = move_stack.top();
    RestoreMoveInfo info = restore_info_stack.top();
    move_stack.pop();
    restore_info_stack.pop();
    board.undo_move<is_white>(info, move);
    state_stack.pop();
}

int Game::reduction(int depth, int i) const {
    int r = std::floor(Parameters::REDUCTION_OFFSET + std::log(double(depth)) * std::log(double(i)) * Parameters::REDUCTION_MULT);
    return std::min(r, depth - 2);
}
void Game::precompute_reduction() {
    for (size_t depth = 1; depth < reduction_depth_cap; depth++) {
        for (size_t i = 1; i < max_legal_moves; i++) {
            reductions[depth][i] = reduction(depth, i);
        }
    }
}
void Game::precompute() { precompute_reduction(); }

void Game::ponderhit() {
    assert(time_manager != nullptr);
    time_manager->ponderhit();
}