// Copyright 2025 Filip Agert
#include "constants.h"
#include "integer_representation.h"
#include "movegen.h"
#include "tables.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <moveorder.h>

#include <board.h>
#include <chrono>
#include <climits>  // For infinity
#include <config.h>
#include <eval.h>
#include <game.h>
#include <iostream>
#include <memory>
#include <move.h>
#include <optional>
#include <string>
#include <time_manager.h>
#include <utility>
bool Game::set_fen(std::string FEN) {
    bool success = board.read_fen(FEN);
    assert(board.board_BB_match());
    reset_state_stack();
    trans_table->clear();
    return success;
}
void Game::start_thinking(const TimeControl rem_time) {
    reset_infos();
    bool is_white = board.get_turn_color() == pieces::white;
    if (is_white)
        think_loop<true>(rem_time);
    else
        think_loop<false>(rem_time);
}

void Game::reset_infos() {
    moves_generated = 0;
    nodes_evaluated = 0;
    bestmove = Move();
}

void Game::reset_state_stack() {
    state_stack.reset();
    uint64_t board_hash = ZobroistHasher::get().hash_board(board);
    state_stack.push(board_hash);
}

constexpr int MATE_BOUND = EvalState::MATE_SCORE - 256;  // anything beyond this is a mate score

inline int score_to_tt(int score, int ply) {
    if (score >  MATE_BOUND) return score + ply;
    if (score < -MATE_BOUND) return score - ply;
    return score;
}
inline int score_from_tt(int score, int ply) {
    if (score >  MATE_BOUND) return score - ply;
    if (score < -MATE_BOUND) return score + ply;
    return score;
}

template <bool is_white> void Game::think_loop(const TimeControl rem_time) {
    if (this->check_repetition()) {
        InfoMsg new_msg;
        new_msg.stringmsg = true;
        new_msg.string = "draw by threefold repetition detected";
        info_queue.push(new_msg);
        return;
    } else if (EvalState::forced_draw_ply(board)) {
        InfoMsg new_msg;
        new_msg.stringmsg = true;
        new_msg.string = "draw by excess ply moves.";
        info_queue.push(new_msg);
        return;
    } else {
        int num_moves = board.get_moves<full_search, is_white>(move_arr[0]);
        if (num_moves == 0) {
            InfoMsg new_msg;
            new_msg.stringmsg = true;
            if (board.king_checked<is_white>()) {
                std::string othercol = is_white ? "black" : "white";
                new_msg.string = "mate detected " + othercol + " has won the game.";
            } else {
                new_msg.string = "draw detected";
            }
            info_queue.push(new_msg);
            return;
        }
        bestmove = move_arr[0][0]; // legal move fallback.
    }
    int buffer = STANDARD_TIME_BUFFER;  // ms
    int fraction = STANDARD_TIME_FRAC;  // spend 1/20th of remaining time.

    time_manager = std::make_shared<TimeManager>(rem_time, buffer, fraction, is_white);

    time_manager->start_time_management();
    int max_depth = 256;
    assert(board.board_BB_match());
    int preveval = 0;
    for (int depth = 1; depth < max_depth; depth++) {
        seldepth = 0;
        if (!time_manager->get_should_start_new_iteration())
            break;
        root_best = Move();
        must_finish = (depth == 1);
        auto [eval, nt] = aspiration_search<is_white>(preveval, depth);
        must_finish = false;
        if (root_best.is_valid())
            bestmove = root_best;
        
        if (nt == timeout)
            break;
        // std::pair<int, uint8_t> result = alpha_beta<true, is_white>(depth, 0, alpha, beta, 0);
        InfoMsg new_msg;
        new_msg.nodes = this->nodes_evaluated;
        new_msg.time = time_manager->get_time_elapsed();
        new_msg.depth = depth;
        new_msg.pv = trans_table->get_pv(board, depth);
        new_msg.seldepth = seldepth;
        new_msg.hashfill = trans_table->load_factor();
        new_msg.score = eval;
        info_queue.push(new_msg);

        preveval = eval;
        std::optional<int> moves_to_mate = EvalState::moves_to_mate(eval);
        if (moves_to_mate) {
            break;
        }
    }

    time_manager->stop_and_join();  // Join time manager thread to this one.
}

template <bool is_white> std::pair<int, uint8_t> Game::aspiration_search(int preveval, int depth) {
    int delta = 25;
    int alpha = -INF;
    int beta = INF;
    if (depth > 1 && settings_.aspiration_enable) {
        alpha = std::max(-INF, preveval - delta);
        beta  = std::min(INF,  preveval + delta);
    }
    int fails = 0;
    while (true) {
        auto res = alpha_beta<true, is_white>(depth, 0, alpha, beta, 0);
        if (res.second == timeout) return res;
        int score = res.first;
        if (score > alpha && score < beta) return res;
        if (score <= alpha) alpha = (++fails > 2) ? -INF : std::max(-INF, score - delta);
        else                beta  = (++fails > 2) ? INF  : std::min(INF,  score + delta);
        delta += delta / 2;
    }
}
template <bool is_root, bool is_white> std::pair<int, uint8_t> Game::alpha_beta(int depth, int ply, int alpha, int beta, int num_extensions) {
    seldepth = std::max(ply, seldepth);
    if (this->check_repetition()) {
        return {0, exact};  // Checks if position is a repeat.
    }
    if (EvalState::forced_draw_ply(board)) {
        return {0, exact};
    }

    if (depth <= 0) {
        nodes_evaluated++;
        return quiesence<is_white>(ply, alpha, beta);
    }
    
    if (ply >= MAX_PLY - 1){
        return {EvalState::eval(board), exact};
    }



    Move best_curr_move;
    bool atleast_one_move_searched = false;
    int bestscore = -INF;
    uint8_t nodetype = ub;
    
    uint64_t zob_hash = state_stack.top();
    std::optional<TranspositionEntry> maybe_entry = trans_table->get(zob_hash);
    std::optional<Move> first_move = {};
    if (maybe_entry) {
        const TranspositionEntry &entry = maybe_entry.value();
        if (entry.is_valid_move())
            first_move = std::make_optional(maybe_entry.value().bestmove);

        if constexpr (!is_root){
            if (trans_table->is_useable_entry(entry, depth)) {
                // If exact, return score
                int eval = score_from_tt(entry.eval, ply);
                if (entry.is_exact()) {  // if exact value
                    return {eval, exact};
                } else if (entry.is_lb()) {
                    if (eval >= beta)  // if its a lower bound, but this lower bound is BETTER than
                        // any move opponent can make
                        return {beta, lb};
                    else
                        alpha = std::max(alpha, eval);  // a lower bound can still tighten alpha./
                } else if (entry.is_ub()) {
                    if (eval < alpha)  // if its an upper bound, and this upper bound is worse
                                       // than any move we could make, dont need to search more.
                        return {alpha, ub};
                    else
                        beta = std::min(beta, eval);  // an upper bound can still tighten beta.
                }
            }
        }
    }
    // Handle if king is checked or no moves can be made.

    int num_moves = board.get_moves<full_search, is_white>(move_arr[ply]);
    moves_generated += num_moves;
    if (num_moves == 0) {
        if (board.king_checked<is_white>()) {
            return {-EvalState::MATE_SCORE + ply, exact};
        } else {
            return {0, exact};
        }
    }
    std::array<Move, max_legal_moves> &moves = move_arr[ply];
    std::array<int, max_legal_moves> &scores = move_heuristic_score[ply];
    MoveOrder::move_heuristics<is_white>(moves, scores, num_moves, board, first_move);



    // Normal move generation.
    //
    for (uint8_t i = 0; i < num_moves; i++) {
        int best = i;
        for (int j = i + 1; j < num_moves; j++)
            if (scores[j] > scores[best]) best = j;
        std::swap(moves[i], moves[best]);
        std::swap(scores[i], scores[best]);     


        make_move<is_white>(moves[i]);
        int extension = calculate_extension<!is_white>(moves[i], i, num_extensions);
        std::pair<int, uint8_t> res = alpha_beta<false, !is_white>(depth - 1 + extension, ply + 1, -beta, -alpha, num_extensions + extension);
        int eval = -res.first;
        undo_move<is_white>();
        if (should_abort()) {
            if constexpr (is_root) {
                if (atleast_one_move_searched && bestscore > alpha) {
                    trans_table->store(zob_hash, best_curr_move, score_to_tt(bestscore, ply), lb, depth);
                }
            }
            return {0, timeout};
        }
        if (eval > bestscore) {
            bestscore = eval;
            best_curr_move = move_arr[ply][i];
            atleast_one_move_searched = true;
        }
        if constexpr (is_root){
            if (eval > alpha) root_best = moves[i];
        }
        if (eval >= beta) {  // FAIL HIGH.
            trans_table->store(zob_hash, best_curr_move, score_to_tt(beta, ply), lb,
                               depth);  // Can update hash to curr depth.
            return {beta, lb};          // This move is too good. The minimising player (beta) will never

            // allow the board to go here. we can return.
        }
        if (eval > alpha) {  // if alpha is raised, then we have found an exact node.
            alpha = eval;    // if alpha is never raised, the value returned will be an upper bound.
            nodetype = exact;
        }
    }
    trans_table->store(zob_hash, nodetype == exact ? best_curr_move : Move(), score_to_tt(alpha, ply), nodetype, depth);
    return {alpha, nodetype};
}

template <bool is_white> std::pair<int, uint8_t> Game::quiesence(int ply, int alpha, int beta) {
    if (this->check_repetition())
        return {0, exact};  // Checks if position is a repeat.
    if (EvalState::forced_draw_ply(board))
        return {0, exact};
    if (ply >= MAX_PLY - 1){
        return {EvalState::eval(board), exact};
    }
    
    uint64_t zob_hash = state_stack.top();
    std::optional<TranspositionEntry> maybe_entry = trans_table->get(zob_hash);
    if (maybe_entry) {
        const TranspositionEntry &entry = maybe_entry.value();
        if (trans_table->is_useable_entry(entry, 0)) {   // depth 0: any stored depth qualifies
            int tt_score = score_from_tt(entry.eval, ply);
            if (entry.is_exact()) return {tt_score, exact};
            if (entry.is_lb() && tt_score >= beta) return {beta, lb};
            if (entry.is_ub() && tt_score <= alpha) return {alpha, ub};
        }
    }

    const bool in_check = board.king_checked<is_white>();
    if (!in_check){
        int eval = EvalState::eval(board);
        if (eval >= beta){
            return {beta, lb};
        }
        alpha = std::max(alpha, eval);
    }
    int num_moves = in_check ?  board.get_moves<full_search, is_white>(move_arr[ply]) : board.get_moves<quiesence_search, is_white>(move_arr[ply]);
    moves_generated += num_moves;
    if (in_check && num_moves == 0)
        return {-EvalState::MATE_SCORE +ply, exact};

    std::array<Move, max_legal_moves> &moves = move_arr[ply];
    std::array<int, max_legal_moves> &scores = move_heuristic_score[ply];
    MoveOrder::move_heuristics<is_white>(moves, scores, num_moves, board, {});
    // Normal move generation.
    for (int i = 0; i < num_moves; i++) {
        int best = i;
        for (int j = i + 1; j < num_moves; j++)
            if (scores[j] > scores[best]) best = j;
        std::swap(moves[i], moves[best]);
        std::swap(scores[i], scores[best]);     

        make_move<is_white>(move_arr[ply][i]);
        std::pair<int, uint8_t> res = quiesence<!is_white>(ply + 1, -beta, -alpha);
        int eval = -res.first;
        undo_move<is_white>();
        if (should_abort()) {
            return {0, timeout};
        }
        if (eval >= beta)
            return {beta, lb};
        alpha = std::max(eval, alpha);
    }

    return {alpha, ub};
}

template <bool is_white> int Game::calculate_extension(const Move move, uint8_t movenum, int num_extensions) const {
    constexpr int max_num_extensions = 16;

    int extension = 0;
    if (num_extensions < max_num_extensions) {
        if (board.king_checked<is_white>())
            extension = 1;
    }
    // if (movenum > 3)
    //     extension -= 1;
    return extension;
}

bool Game::check_repetition() {
    // Checks if we have repeated this board state.
    uint64_t hash = state_stack.top();
    constexpr int instances_for_draw = 3;  // How many occurences of this board should have occured for a draw?
    bool atleast2 = state_stack.atleast_num(hash, instances_for_draw);
    return atleast2;
}
Move Game::get_bestmove() const { return bestmove; }

std::string Game::get_fen() const { return board.fen_from_state(); }

template <bool is_white> void Game::make_move(Move move) {
    RestoreMoveInfo info = board.do_move<is_white>(move);
    move_stack.push(move);
    restore_info_stack.push(info);
    state_stack.push(state_stack.top() ^ ZobroistHasher::get().delta(info.delta));
    assert(state_stack.top () == ZobroistHasher::get().hash_board(board));
}

template <bool is_white> void Game::undo_move() {
    Move move = move_stack.top();
    RestoreMoveInfo info = restore_info_stack.top();
    move_stack.pop();
    restore_info_stack.pop();
    board.undo_move<is_white>(info, move);
    state_stack.pop();
}
