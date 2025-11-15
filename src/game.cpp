#include <game.h>
#include <string>
#include <stdexcept>


bool Game::set_fen(std::string FEN){
    bool success = NotationInterface::read_fen(FEN, this->state);
    return success;
}

void Game::display_board() {
    const char* files = "A B C D E F G H";

    std::cout << "    " << files << std::endl;
    std::cout << "  +-----------------+" << std::endl;
    uint8_t ep_sq = err_val8;
    bool ep = this->state.en_passant;
    if(ep) ep_sq = this->state.en_passant_square;

    for (int r = 7; r >= 0; --r) {
        std::cout << r + 1 << " | ";
        for (int c = 0; c < 8; ++c) {
            uint8_t idx = Board::idx(r, c);
            Piece p = this->state.board.get_piece_at(idx);
            char printval = p.get_char();
            if (printval == ' ' && ep && ep_sq == idx) printval = 'x';
            std::cout << printval << ' ';

        }
        std::cout << "| " << r + 1 << "     ";
        switch (r) { 
            case 7: {
                std::cout << "Turn: " << Piece::char_from_val(this->state.turn_color); break;
            }
            case 6: {
                std::cout << "Ply counter: " << this->state.ply_moves; break;    
            } 
            case 5: {
                std::cout << "Move counter: " << this->state.full_moves; break;    
            } 
            case 4: {
                std::cout << "En passant: " << this->state.en_passant; break;    
            } 
            case 3: {
                std::cout << "Castle rights: " << NotationInterface::castling_rights(this->state.castling); break;
            }
            case 2: {
                std::cout << "Number of white pieces: (not implemented)"; break;
            }
            case 1: {
                std::cout << "Number of black pieces: (not implemented)"; break;
            }
        }
        std::cout<<std::endl;
    }

    std::cout << "  +-----------------+" << std::endl;
    std::cout << "    " << files << std::endl;
}
