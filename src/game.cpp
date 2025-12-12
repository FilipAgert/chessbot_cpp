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
    reset_state_stack();
    trans_table->clear();
    return success;
}
void Game::start_thinking(const time_control rem_time) {
    reset_infos();
    think_loop(rem_time);
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

void Game::think_loop(const time_control rem_time) {
    int buffer = STANDARD_TIME_BUFFER;  // ms
    int fraction = STANDARD_TIME_FRAC;  // spend 1/20th of remaining time.

    time_manager = std::make_shared<TimeManager>(rem_time, buffer, fraction,
                                                 board.get_turn_color() == pieces::white);

    time_manager->start_time_management();
    int max_depth = 256;
    uint64_t hash = ZobroistHasher::get().hash_board(board);
    for (int depth = 1; depth < max_depth; depth++) {
        if (!time_manager->get_should_start_new_iteration())
            break;
        int alpha = -INF;
        const int beta = INF;
        alpha_beta<true>(depth, 0, alpha, beta, 0);
        InfoMsg new_msg;
        new_msg.nodes = this->nodes_evaluated;
        new_msg.time = time_manager->get_time_elapsed();
        new_msg.depth = depth;
        new_msg.pv = trans_table->get_pv(board, depth);
        if (new_msg.pv.size() > 0) {
            bestmove = new_msg.pv[0];
            if (bestmove.source == bestmove.target) {
                std::cout << "Illegal move made." << std::endl;
            }
        }

        std::optional<transposition_entry> entry = trans_table->get(hash);
        if (entry) {
            new_msg.score = entry.value().eval;
        } else {
            std::cout << "Err: could not get entry for hash" << std::endl;
            new_msg.score = 0;
        }

        info_queue.push(new_msg);
        std::optional<int> moves_to_mate = EvalState::moves_to_mate(alpha);
        if (moves_to_mate) {
            break;
        }
    }

    time_manager->stop_and_join();  // Join time manager thread to this one.
}

template <bool is_root>
int Game::alpha_beta(int depth, int ply, int alpha, int beta, int num_extensions) {
    // if (this->check_repetition())
    //     return 0;  // Checks if position is a repeat.
    if (EvalState::forced_draw_ply(board))
        return 0;

    if (depth <= 0) {
        nodes_evaluated++;
        return quiesence(ply, alpha, beta);
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
        // if (trans_table->is_useable_entry(entry, depth)) {
        //     // If exact, return score
        //     int eval = entry.eval;   //
        //     if (entry.is_exact()) {  // if exact value
        //         return eval;
        //     } else if (entry.is_lb()) {
        //         if (eval >= beta)  // if its a lower bound, but this lower bound is BETTER than
        //         any
        //                            // move opponent can make
        //             return beta;
        //         else
        //             alpha = std::max(alpha, eval);  // a lower bound can still tighten alpha./
        //     } else if (entry.is_ub()) {
        //         if (eval < alpha)  // if its an upper bound, and this upper bound is worse
        //                            // than any move we could make, dont need to search more.
        //             return alpha;
        //         else
        //         beta = std::min(beta, eval);  // an upper bound can still tighten beta.
        //     }
        // }
        // If the transposition table entry was not useable due to bad depth, or if it was not
        // enough to produce a cutoff, it can still be used for move ordering.
        first_move = std::make_optional(entry.bestmove);
        // We first search the first move before generating the other moves. If it generates a
        // cutoff, we save plenty of time.
        if (entry.is_valid_move()) {  // need ot check if its a valid move or not, since it might be
                                      // e.g. no moves available on this state.
            int extension = calculate_extension(entry.bestmove, num_extensions);
            make_move(entry.bestmove);
            int eval =
                -alpha_beta<false>(depth - 1, ply + 1, -beta, -alpha, num_extensions + extension);
            undo_move();
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
    std::array<Move, max_legal_moves> moves;
    int num_moves = board.get_moves<normal_search>(moves);
    moves_generated += num_moves;
    if (num_moves == 0) {
        const int MATE_SCORE = 30000;
        if (board.king_checked(board.get_turn_color())) {
            return (-MATE_SCORE + ply);
        } else {
            return 0;
        }
    }
    // End mate and draws.

    MoveOrder::apply_move_sort(moves, num_moves, first_move, board);
    // Normal move generation.
    //
    for (int i = movelb; i < num_moves; i++) {
        this->make_move(moves[i]);
        int extension = calculate_extension(moves[i], num_extensions);

        int eval = -alpha_beta<false>(depth - 1 + extension, ply + 1, -beta, -alpha,
                                      num_extensions + extension);
        this->undo_move();
        if (time_manager->get_should_stop()) {
            if (is_root) {
                if (atleast_one_move_searched) {
                    trans_table->store(zob_hash, best_curr_move, bestscore, transposition_entry::lb,
                                       depth);
                }
            }
            return 0;
        }
        if (eval > bestscore) {
            bestscore = eval;
            best_curr_move = moves[i];
            atleast_one_move_searched = true;
        }
        if (eval >= beta) {  // FAIL HIGH.
            trans_table->store(zob_hash, best_curr_move, beta, transposition_entry::lb,
                               depth);  // Can update hash to curr depth.
            return beta;  // This move is too good. The minimising player (beta) will never
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

int Game::quiesence(int ply, int alpha, int beta) {
    if (this->check_repetition())
        return 0;  // Checks if position is a repeat.
    if (EvalState::forced_draw_ply(board))
        return 0;

    int eval = EvalState::eval(board);

    // This assumes that there is at least one move that can match, or increase the current score.
    // So best_value is a lower bound.
    if (eval >= beta)
        return beta;
    alpha = std::max(alpha, eval);

    // Handle if king is checked or no moves can be made.
    std::array<Move, max_legal_moves> moves;
    int num_moves = board.get_moves<quiesence_search>(moves);
    moves_generated += num_moves;

    MoveOrder::apply_move_sort(moves, num_moves, board);
    // Normal move generation.
    for (int i = 0; i < num_moves; i++) {
        this->make_move(moves[i]);
        eval = -quiesence(ply + 1, -beta, -alpha);
        this->undo_move();
        if (eval >= beta)
            return beta;
        alpha = std::max(eval, alpha);
    }

    return alpha;
}

int Game::calculate_extension(const Move &move, int num_extensions) const {
    constexpr int max_num_extensions = 16;

    int extension = 0;
    if (num_extensions < max_num_extensions) {
        if (board.king_checked(board.get_turn_color()))
            extension = 1;
    }
    return extension;
}

bool Game::check_repetition() {
    // Checks if we have repeated this board state.
    uint64_t hash = state_stack.top();
    constexpr int instances_for_draw =
        2;  // How many occurences of this board should have occured for a draw?
    bool atleast2 = state_stack.atleast_num(hash, instances_for_draw);
    return atleast2;
}
Move Game::get_bestmove() const { return bestmove; }

std::string Game::get_fen() const { return board.fen_from_state(); }

void Game::make_move(Move move) {
    board.do_move(move);
    move_stack.push(move);
    uint64_t state_hash = ZobroistHasher::get().hash_board(board);
    state_stack.push(state_hash);
}

void Game::undo_move() {
    Move move = move_stack.top();
    move_stack.pop();
    board.undo_move(move);
    state_stack.pop();
}
