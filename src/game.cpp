#include <game.h>
#include <string>
#include <stdexcept>


bool Game::set_fen(std::string FEN){
    bool success = NotationInterface::read_fen(FEN, this->state);
    return success;
}