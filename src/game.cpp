// Copyright 2025 Filip Agert
#include <game.h>
#include <string>

bool Game::set_fen(std::string FEN) {
    bool success = state.read_fen(FEN);
    return success;
}

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
