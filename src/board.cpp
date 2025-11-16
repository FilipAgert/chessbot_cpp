#include <board.h>

uint8_t Board::idx_from_string(std::string square){
    if (square.length() != 2) { //Needs exactly two characters
        return err_val8;
    }
    char colchar = square[0];
    char rowchar = square[1];
    uint8_t basecol;
    uint8_t baserow;
    uint8_t colval;
    uint8_t rowval;

    if(colchar >= 'A' && colchar <= 'H') {
        basecol = 'A';
    } else if(colchar >= 'a' && colchar <= 'h') {
        basecol = 'a';
    } else {
        return err_val8;
    }
    colval =colchar - basecol;
    if(rowchar >= '1' && rowchar <= '8') {
        baserow = '1';
        rowval =rowchar - baserow;
    } else {
        return err_val8;
    }

    return idx(rowval, colval);
};

std::string Board::string_from_idx(const uint8_t idx){
    uint8_t row, col;
    row_col(row, col, idx);
    char r = 'a'+col;
    char c = '1'+row;
    std::string s = std::string() + r + c;
    return s;
}
void Board::row_col(uint8_t& row, uint8_t& col, const uint8_t idx){
    col = idx % 8;
    row = idx/8;
}

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

