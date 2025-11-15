#include <game.h>
#include <string>
#include <stdexcept>


bool Game::set_fen(std::string FEN){
    bool success = NotationInterface::read_fen(FEN, this->state);
    return success;
}

void Game::display_board(){
    for(int r = 7; r>-1; r--){
        for(int c = 0; c<8; c++){
            uint8_t idx = Board::idx(r, c);
            Piece p = this->state.board.get_piece_at(idx);
            std::cout<<p.get_char();
        }
        std::cout<<std::endl;
    }
}