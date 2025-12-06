// Copyright 2025 Filip Agert
#include <algorithm>
#include <array>
#include <chrono>
#include <climits>  // For infinity
#include <config.h>
#include <eval.h>
#include <game.h>
#include <memory>
#include <move.h>
#include <string>
#include <time_manager.h>
bool Game::set_fen(std::string FEN) {
    bool success = state.read_fen(FEN);
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

void Game::think_loop(const time_control rem_time) {
    int buffer = STANDARD_TIME_BUFFER;  // ms
    int fraction = STANDARD_TIME_FRAC;  // spend 1/20th of remaining time.

    std::array<Move, max_legal_moves> moves;
    int num_moves = state.get_moves(moves);
    if (num_moves == 0)
        return;

    std::shared_ptr<TimeManager> time_manager(
        new TimeManager(rem_time, buffer, fraction, this->state.turn_color == pieces::white));
    time_manager->start_time_management();

    std::array<int, max_legal_moves> evaluations;
    bool is_maximiser = state.turn_color == pieces::white;

    int depth = 1;
    bool ponder = true;

    while (ponder) {
        Move best_current_move;
        int eval;
        int best_eval;
        int init_alpha = INT_MIN;
        int init_beta = INT_MAX;
        size_t num_moves_evaluated = 0;

        if (is_maximiser) {  // TODO: Make only one call to alpha_beta.
            best_eval = INT_MIN;
            for (int i = 0; i < num_moves; i++) {
                this->make_move(moves[i]);
                eval = alpha_beta(depth - 1, init_alpha, init_beta, false);
                evaluations[i] = eval;
                this->undo_move();
                if (eval > best_eval) {
                    best_eval = eval;
                    best_current_move = moves[i];
                }
                num_moves_evaluated++;
                if (time_manager->get_should_stop()) {
                    // Even if we break early we get information from this evaluation.
                    // Due to the move ordering after each depth, we search the best moves first.
                    // This means that even if we only evaluate say the top 3 moves out of 20
                    // possible, we will get the best one of these 3 at the current depth. This is
                    // good.
                    ponder = false;
                    break;
                }
            }
        } else {
            best_eval = INT_MAX;
            for (int i = 0; i < num_moves; i++) {
                this->make_move(moves[i]);
                eval = alpha_beta(depth - 1, init_alpha, init_beta, true);
                evaluations[i] = eval;
                this->undo_move();
                if (eval < best_eval) {
                    best_eval = eval;
                    best_current_move = moves[i];
                }
                num_moves_evaluated++;
                if (time_manager->get_should_stop()) {
                    ponder = false;
                    break;
                }
            }
        }
        EvalState::partial_move_sort(moves, evaluations, num_moves_evaluated,
                                     !is_maximiser);  // Sort moves by score in order to help next
                                                      // depth improve move ordering.
        bestmove = moves[0];

        InfoMsg new_msg;
        new_msg.nodes = this->nodes_evaluated;
        new_msg.time = time_manager->get_time_elapsed();
        new_msg.depth = depth;
        new_msg.pv = {bestmove};
        new_msg.score = best_eval;
        info_queue.push(new_msg);
        // Sort move list by the score list.
        depth++;
    }

    time_manager->stop_and_join();  // Join time manager thread to this one.
}

int Game::alpha_beta(size_t depth, int alpha, int beta, bool is_maximiser) {
    if (depth == 0) {
        nodes_evaluated++;
        return EvalState::eval(this->state);
    }

    int eval;
    std::array<Move, max_legal_moves> moves;
    int num_moves = state.get_moves(moves);
    if (is_maximiser) {  // white
        int max_eval = INT_MIN;
        moves_generated += num_moves;
        for (int i = 0; i < num_moves; i++) {
            this->make_move(moves[i]);
            eval = alpha_beta(depth - 1, alpha, beta, false);
            this->undo_move();
            max_eval = std::max(max_eval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) {
                break;  // beta cutoff.
            }
        }
        return max_eval;
    } else {  // black
        int min_eval = INT_MAX;
        moves_generated += num_moves;
        for (int i = 0; i < num_moves; i++) {
            this->make_move(moves[i]);
            eval = alpha_beta(depth - 1, alpha, beta, true);
            this->undo_move();
            min_eval = std::min(min_eval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) {
                break;  // Alpha cutoff.
            }
        }
        return min_eval;
    }
}

Move Game::get_bestmove() const { return bestmove; }

void Game::reset_game() { bool success = state.read_fen(NotationInterface::starting_FEN()); }
std::string Game::get_fen() const { return ""; }

void Game::make_move(Move move) {
    state.do_move(move);
    move_stack.push(move);
}

void Game::undo_move() {
    Move move = move_stack.top();
    move_stack.pop();
    state.undo_move(move);
}
