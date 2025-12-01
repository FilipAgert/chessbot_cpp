// Copyright 2025 Filip Agert
#include <array>
#include <chrono>
#include <game.h>
#include <move.h>
#include <string>
#include <time_manager.h>
bool Game::set_fen(std::string FEN) {
    bool success = state.read_fen(FEN);
    return success;
}
void Game::start_thinking() {
    std::array<Move, max_legal_moves> moves;
    size_t num_moves = state.get_moves(moves);
    if (num_moves > 0)
        bestmove = moves[0];
    else
        bestmove = Move("a2a4");
}

void Game::think_loop() {
    int max_depth = 5;

    TimeManager time_manager =
        TimeManager(3000, 500, 10000, 50);  // Todo: Input actual args from UCI.
    time_manager.start_time_management();

    for (int depth = 1; depth <= max_depth; depth++) {
        if (time_manager.get_should_stop()) {
            break;
        }
    }

    if (!time_manager.get_should_stop()) {
        time_manager.set_should_stop(true);
        time_manager.stop_and_join();
    }
}

std::string Game::get_bestmove() const { return bestmove.toString(); }

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
