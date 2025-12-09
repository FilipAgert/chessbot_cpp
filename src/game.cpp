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
#include <utility>
bool Game::set_fen(std::string FEN) {
    bool success = board.read_fen(FEN);
    reset_state_stack();
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

    std::array<Move, max_legal_moves> moves;
    int num_moves = board.get_moves<normal_search>(moves);
    if (num_moves == 0)

        return;

    MoveOrder::apply_move_sort(moves, num_moves, board);
    std::shared_ptr<TimeManager> time_manager(
        new TimeManager(rem_time, buffer, fraction, board.get_turn_color() == pieces::white));
    time_manager->start_time_management();

    int depth = 1;
    bool ponder = true;

    while (ponder) {
        int eval;
        int alpha = -INF;
        const int beta = INF;
        int best_move_idx = 0;
        for (int i = 0; i < num_moves; i++) {
            this->make_move(moves[i]);
            eval = -alpha_beta(depth - 1, 1, -beta, -alpha, 0);
            this->undo_move();
            if (eval > alpha) {
                alpha = eval;
                best_move_idx = i;  //
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
        std::swap(moves[0], moves[best_move_idx]);  // Set best move first.

        bestmove = moves[0];

        InfoMsg new_msg;
        new_msg.nodes = this->nodes_evaluated;
        new_msg.time = time_manager->get_time_elapsed();
        new_msg.depth = depth;
        new_msg.pv = {bestmove};
        new_msg.score = alpha;
        info_queue.push(new_msg);
        depth++;
        std::optional<int> moves_to_mate = EvalState::moves_to_mate(alpha);
        if (moves_to_mate) {
            break;
        }
    }

    time_manager->stop_and_join();  // Join time manager thread to this one.
}

int Game::alpha_beta(int depth, int ply, int alpha, int beta, int num_extensions) {
    if (this->check_repetition())
        return 0;  // Checks if position is a repeat.

    if (depth == 0) {
        nodes_evaluated++;
        return quiesence(ply, alpha, beta);
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
    //
    for (int i = 0; i < num_moves; i++) {
        this->make_move(moves[i]);
        int extension = calculate_extension(moves[i], num_extensions);

        eval =
            -alpha_beta(depth - 1 + extension, ply + 1, -beta, -alpha, num_extensions + extension);
        this->undo_move();
        if (eval >= beta) {
            return beta;
        }
        alpha = std::max(alpha, eval);
    }

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
