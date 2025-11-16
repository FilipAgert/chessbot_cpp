#include <game.h>
#include <string>
#include <stdexcept>


bool Game::set_fen(std::string FEN){
    bool success = NotationInterface::read_fen(FEN, this->state);
    return success;
}

std::string Game::get_fen() const{
    return "";
}


void Game::make_move(const uint8_t start_square, const uint8_t end_square){
    Move move;
    move.start_square = start_square;
    move.end_square = end_square;
    state.do_move(move);
    move_stack.push(move);
}

void Game::undo_move(){
    Move move = move_stack.top();
    move_stack.pop();
    state.undo_move(move);
}