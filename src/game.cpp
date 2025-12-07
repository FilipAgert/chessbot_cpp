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
#include <memory>
#include <move.h>
#include <string>
#include <time_manager.h>
bool Game::set_fen(std::string FEN) {
    bool success = board.read_fen(FEN);
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
    int num_moves = board.get_moves<normal_search>(moves);
    if (num_moves == 0)

        return;

    std::shared_ptr<TimeManager> time_manager(
        new TimeManager(rem_time, buffer, fraction, board.get_turn_color() == pieces::white));
    time_manager->start_time_management();

    std::array<int, max_legal_moves> evaluations;

    int depth = 1;
    bool ponder = true;

    while (ponder) {
        int eval;
        int best_eval;
        size_t num_moves_evaluated = 0;

        best_eval = INT_MIN;
        for (int i = 0; i < num_moves; i++) {
            this->make_move(moves[i]);
            eval = -alpha_beta(depth - 1, 1, -INF, INF);
            evaluations[i] = eval;
            this->undo_move();
            best_eval = std::max(eval, best_eval);
            num_moves_evaluated++;
            std::optional<int> moves_to_mate = EvalState::moves_to_mate(best_eval);

            if (moves_to_mate) {  // if we have mate, dont need to check further.
                if (moves_to_mate.value() >
                    0) {  // Negative value means we are getting mated. ofc check all moves.
                    ponder = false;
                    break;
                }
            }

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
        std::optional<int> moves_to_mate = EvalState::moves_to_mate(best_eval);

        MoveOrder::partial_move_sort(moves, evaluations, num_moves_evaluated,
                                     false);  // Sort moves by score in order to help next
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
        if (moves_to_mate) {
            break;
        }
    }

    time_manager->stop_and_join();  // Join time manager thread to this one.
}

int Game::alpha_beta(int depth, int ply, int alpha, int beta) {
    if (depth == 0) {
        nodes_evaluated++;
        return EvalState::eval(board);
    }

    if (EvalState::forced_draw_ply(board))
        return 0;

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

    MoveOrder::apply_move_sort(moves, num_moves, board);
    int eval = -INF;
    // Normal move generation.
    for (int i = 0; i < num_moves; i++) {
        this->make_move(moves[i]);
        eval = std::max(eval, -alpha_beta(depth - 1, ply + 1, -beta, -alpha));
        alpha = std::max(alpha, eval);
        this->undo_move();
        if (eval >= beta) {
            break;  // fail soft.
        }
    }

    return eval;
}

Move Game::get_bestmove() const { return bestmove; }

void Game::reset_game() { bool success = board.read_fen(NotationInterface::starting_FEN()); }
std::string Game::get_fen() const { return board.fen_from_state(); }

void Game::make_move(Move move) {
    board.do_move(move);
    move_stack.push(move);
}

void Game::undo_move() {
    Move move = move_stack.top();
    move_stack.pop();
    board.undo_move(move);
}
