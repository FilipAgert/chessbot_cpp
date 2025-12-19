// Copyright 2025 Filip Agert
#include <algorithm>
#include <array>
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
void Game::start_thinking(const time_control rem_time) {
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

template <bool is_white> void Game::think_loop(const time_control rem_time) {
    if (check_repetition()) {
        InfoMsg new_msg;
        new_msg.stringmsg = true;
        new_msg.string = "draw by threefold repetition detected";
    } else {
        int num_moves = board.get_moves<normal_search, is_white>(move_arr[0]);
        if (num_moves == 0) {
            InfoMsg new_msg;
            new_msg.stringmsg = true;
            if (board.king_checked<is_white>()) {
                std::string othercol = is_white ? "black" : "white";
                new_msg.string = "mate detected " + othercol + " has won the game.";
            } else {
                new_msg.string = "draw detected";
            }
        }
    }
    int buffer = STANDARD_TIME_BUFFER;  // ms
    int fraction = STANDARD_TIME_FRAC;  // spend 1/20th of remaining time.

    time_manager = std::make_shared<TimeManager>(rem_time, buffer, fraction, is_white);

    time_manager->start_time_management();
    int max_depth = 256;
    uint64_t hash = ZobroistHasher::get().hash_board(board);
    assert(board.board_BB_match());
    for (int depth = 1; depth < max_depth; depth++) {
        seldepth = 0;
        if (!time_manager->get_should_start_new_iteration())
            break;
        int alpha = -INF;
        const int beta = INF;
        alpha_beta<true, is_white>(depth, 0, alpha, beta, 0);
        InfoMsg new_msg;
        new_msg.nodes = this->nodes_evaluated;
        new_msg.time = time_manager->get_time_elapsed();
        new_msg.depth = depth;
        new_msg.pv = trans_table->get_pv<is_white>(board, depth);
        new_msg.seldepth = seldepth;
        new_msg.hashfill = trans_table->load_factor();
        if (new_msg.pv.size() > 0) {
            bestmove = new_msg.pv[0];
            if (bestmove.source == bestmove.target) {
                std::cout << "Illegal move made." << std::endl;
            }
        }

        std::optional<transposition_entry> entry = trans_table->get(hash);
        std::optional<int> eval = {};
        if (entry) {
            new_msg.score = entry.value().eval;
            info_queue.push(new_msg);
            eval = std::make_optional(new_msg.score);
        } else {
            std::cout << "Err: could not get entry for hash. Depth: " << depth << std::endl;
        }

        std::optional<int> moves_to_mate = eval ? EvalState::moves_to_mate(eval.value()) : 0;
        if (moves_to_mate) {
            break;
        }
    }

    time_manager->stop_and_join();  // Join time manager thread to this one.
}

template <bool is_root, bool is_white> int Game::alpha_beta(int depth, int ply, int alpha, int beta, int num_extensions) {
    seldepth = std::max(ply, seldepth);
    if (this->check_repetition())
        return 0;  // Checks if position is a repeat.
    if (EvalState::forced_draw_ply(board))
        return 0;

    if (depth <= 0) {
        nodes_evaluated++;
        return quiesence<is_white>(ply, alpha, beta);
    }

    uint64_t zob_hash = ZobroistHasher::get().hash_board(board);
    std::optional<transposition_entry> maybe_entry = trans_table->get(zob_hash);
    std::optional<Move> first_move = {};
    int movelb = 0;
    Move best_curr_move;
    bool atleast_one_move_searched = false;
    int bestscore = -INF;
    uint8_t nodetype = transposition_entry::ub;
    if (maybe_entry) {
        transposition_entry entry = maybe_entry.value();
        if (trans_table->is_useable_entry(entry, depth)) {
            // If exact, return score
            int eval = entry.eval;
            if (entry.is_exact()) {  // if exact value
                return eval;
            } else if (entry.is_lb()) {
                if (eval >= beta)  // if its a lower bound, but this lower bound is BETTER than
                    // any move opponent can make
                    return beta;
                else
                    alpha = std::max(alpha, eval);  // a lower bound can still tighten alpha./
            } else if (entry.is_ub()) {
                if (eval < alpha)  // if its an upper bound, and this upper bound is worse
                                   // than any move we could make, dont need to search more.
                    return alpha;
                else
                    beta = std::min(beta, eval);  // an upper bound can still tighten beta.
            }
        }
        // If the transposition table entry was not useable due to bad depth, or if it was not
        // enough to produce a cutoff, it can still be used for move ordering.
        first_move = std::make_optional(entry.bestmove);
        // We first search the first move before generating the other moves. If it generates a
        // cutoff, we save plenty of time.
        if (entry.is_valid_move()) {  // need ot check if its a valid move or not, since it might be
                                      // e.g. no moves available on this state.
            make_move<is_white>(entry.bestmove);
            int extension = calculate_extension<!is_white>(entry.bestmove, 0, num_extensions);
            int eval = -alpha_beta<false, !is_white>(depth - 1, ply + 1, -beta, -alpha, num_extensions + extension);
            undo_move<is_white>();
            if (time_manager->get_should_stop()) {
                return 0;  // should not store into transposition table here since the search was
                           // cancelled.
            }
            bestscore = eval;
            best_curr_move = entry.bestmove;
            atleast_one_move_searched = true;
            if (eval >= beta) {  // FAIL HIGH: move is too good, will never get here.
                trans_table->store(zob_hash, entry.bestmove, beta, transposition_entry::lb,
                                   depth);  // Can update hash to curr depth.
                return beta;
            }

            if (eval > alpha) {  // alpha raised and node is exact.
                alpha = eval;
                nodetype = transposition_entry::exact;
            }
            movelb = 1;
        }
    }

    // Handle if king is checked or no moves can be made.
    int num_moves = board.get_moves<normal_search, is_white>(move_arr[ply]);
    moves_generated += num_moves;
    if (num_moves == 0) {
        const int MATE_SCORE = 30000;
        if (board.king_checked<is_white>()) {
            return (-MATE_SCORE + ply);
        } else {
            return 0;
        }
    }
    // End mate and draws.

    MoveOrder::apply_move_sort<is_white>(move_arr[ply], num_moves, first_move, board);
    // Normal move generation.
    //
    for (uint8_t i = movelb; i < num_moves; i++) {
        make_move<is_white>(move_arr[ply][i]);
        int extension = calculate_extension<!is_white>(move_arr[ply][i], i, num_extensions);

        int eval = -alpha_beta<false, !is_white>(depth - 1 + extension, ply + 1, -beta, -alpha, num_extensions + extension);
        undo_move<is_white>();
        if (time_manager->get_should_stop()) {
            if (is_root) {
                if (atleast_one_move_searched) {
                    trans_table->store(zob_hash, best_curr_move, bestscore, transposition_entry::lb, depth);
                }
            }
            return 0;
        }
        if (eval > bestscore) {
            bestscore = eval;
            best_curr_move = move_arr[ply][i];
            atleast_one_move_searched = true;
        }
        if (eval >= beta) {  // FAIL HIGH.
            trans_table->store(zob_hash, best_curr_move, beta, transposition_entry::lb,
                               depth);  // Can update hash to curr depth.
            return beta;                // This move is too good. The minimising player (beta) will never
                                        // allow the board to go here. we can return.
        }
        if (eval > alpha) {  // if alpha is raised, then we have found an exact node.
            alpha = eval;    // if alpha is never raised, the value returned will be an upper bound.
            nodetype = transposition_entry::exact;
        }
    }
    trans_table->store(zob_hash, best_curr_move, alpha, nodetype, depth);
    return alpha;
}

template <bool is_white> int Game::quiesence(int ply, int alpha, int beta) {
    if (this->check_repetition())
        return 0;  // Checks if position is a repeat.
    if (EvalState::forced_draw_ply(board))
        return 0;

    int eval = EvalState::eval(board);
    seldepth = std::max(ply, seldepth);

    // This assumes that there is at least one move that can match, or increase the current score.
    // So best_value is a lower bound.
    if (eval >= beta)
        return beta;
    alpha = std::max(alpha, eval);

    // Handle if king is checked or no moves can be made.
    int num_moves = board.get_moves<quiesence_search, is_white>(move_arr[ply]);
    moves_generated += num_moves;

    MoveOrder::apply_move_sort<is_white>(move_arr[ply], num_moves, board);
    // Normal move generation.
    for (int i = 0; i < num_moves; i++) {
        make_move<is_white>(move_arr[ply][i]);
        eval = -quiesence<!is_white>(ply + 1, -beta, -alpha);
        undo_move<is_white>();
        if (time_manager->get_should_stop()) {
            return 0;
        }
        if (eval >= beta)
            return beta;
        alpha = std::max(eval, alpha);
    }

    return alpha;
}

template <bool is_white> int Game::calculate_extension(const Move move, uint8_t movenum, int num_extensions) const {
    constexpr int max_num_extensions = 16;

    int extension = 0;
    if (num_extensions < max_num_extensions) {
        if (board.king_checked<is_white>())
            extension = 1;
    }
    if (movenum > 3)
        extension -= 1;
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
    restore_move_info info = board.do_move<is_white>(move);
    move_stack.push(move);
    restore_info_stack.push(info);
    uint64_t state_hash = ZobroistHasher::get().hash_board(board);
    state_stack.push(state_hash);
}

template <bool is_white> void Game::undo_move() {
    Move move = move_stack.top();
    restore_move_info info = restore_info_stack.top();
    move_stack.pop();
    restore_info_stack.pop();
    board.undo_move<is_white>(info, move);
    state_stack.pop();
}
