#include <board.h>


bool Board::is_square_empty(uint8_t square)const{
    return this->get_piece_at(square) == none_piece;
}

uint8_t Board::get_square_color(uint8_t square)const{
    return this->get_piece_at(square).get_color();
}
    
void Board::move_piece(uint8_t start_square, uint8_t end_square){
    this->add_piece(end_square, this->get_piece_at(start_square));
    this->remove_piece(start_square);
}

void Board::add_piece(uint8_t square, Piece piece){
    this->game_board[square] = piece;
}

void Board::remove_piece(uint8_t square){
    this->game_board[square] = Piece();
}

void Board::clear_board(){
    for (int i = 0; i<64; i++){
        this->game_board[i] = Piece();
    }
}

bool Board::operator==(const Board& other)const{
    for (int i = 0; i<64;i++) if (!(game_board[i] == other.game_board[i])) return false; 
    return true;
}

Board::Board(){
    for (int i = 0; i<64; i++){
        game_board[i] = Piece();
    }
}

Board::~Board(){
}

