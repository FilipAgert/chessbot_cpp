#include <game.h>
#include <string>
#include <stdexcept>


void Game::set_fen(std::string FEN){
    bool success = NotationInterface::read_fen(FEN, this->state);
    
    if(!success) throw std::invalid_argument("Invalid FEN state.");
}