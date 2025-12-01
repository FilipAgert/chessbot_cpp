// Copyright 2025 Filip Agert
#include <array>
#include <game.h>
#include <move.h>
#include <string>

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
