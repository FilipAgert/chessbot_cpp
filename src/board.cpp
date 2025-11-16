#include <board.h>


void Board::piece_loc_move(uint8_t from, uint8_t to){
    uint8_t idx = 0;
    while (idx < num_pieces){
        if(piece_locations[idx] == from){
            piece_locations[idx] = to;
            return;
        }
        idx++;
    }
    std::cerr << "No piece found at square: " <<(int) from<< " " << NotationInterface::string_from_idx(from) << std::endl;
    std::cerr << "Attempted move: " << Move(from, to).toString()<< std::endl;
    std::cerr << "Number of pieces: " << (int)num_pieces << std::endl;
    std::cerr << "Locations in piece_loc:"<<std::endl;
    for(int i =0; i<num_pieces; i++) std::cerr << NotationInterface::string_from_idx(piece_locations[i]) << " ";
    std::cerr << std::endl;
    std::abort();
}
void Board::piece_loc_remove(uint8_t sq){
    uint8_t idx = 0;
    while (idx < num_pieces){
        if(piece_locations[idx] == sq) break;
        idx++;
    }
    idx++;
    while (idx < num_pieces){
        piece_locations[idx-1] = piece_locations[idx];
        idx++;
    }
}

bool Board::is_square_empty(uint8_t square)const{
    return this->get_piece_at(square) == none_piece;
}

uint8_t Board::get_square_color(uint8_t square)const{
    return this->get_piece_at(square).get_color();
}

void Board::undo_capture(const uint8_t start_square,const  uint8_t end_square, const Piece captured){
    game_board[start_square] = game_board[end_square];
    game_board[end_square] = captured;
    piece_loc_remove(start_square);//Counterintuative, but the captured piece already points to this location
    num_pieces++;
}
void Board::undo_capture_ep(const uint8_t start_square,const  uint8_t end_square, const Piece captured, const uint8_t captured_pawn_loc){
    move_piece(end_square, start_square);
    add_piece(captured_pawn_loc, captured);
}
void Board::capture_piece(const uint8_t start_square,const  uint8_t end_square){
    game_board[end_square] = game_board[start_square];
    game_board[start_square] = none_piece;
    piece_loc_remove(start_square);//Counterintuative, but the captured piece already points to this location
    num_pieces--;
}
void Board::capture_piece_ep(const uint8_t start_square, const uint8_t end_square, uint8_t captured_pawn_loc){
    game_board[end_square] = game_board[start_square];
    game_board[start_square] = none_piece;
    game_board[captured_pawn_loc] = none_piece;
    piece_loc_move(start_square, end_square);
    piece_loc_remove(captured_pawn_loc);
    num_pieces--;
}
    
void Board::move_piece(const uint8_t start_square,const  uint8_t end_square){
    game_board[end_square] = game_board[start_square];
    game_board[start_square] = none_piece;
    piece_loc_move(start_square, end_square);
}

void Board::add_piece(const uint8_t square,const  Piece piece){
    game_board[square] = piece;
    piece_loc_add(square);
    num_pieces++;
}

void Board::remove_piece(const uint8_t square){
    game_board[square] = none_piece;
    piece_loc_remove(square);
    num_pieces--;
}
void Board::promote_piece(const uint8_t square, const Piece promotion){
    game_board[square] = promotion;
}

void Board::clear_board(){
    piece_locations.fill(0);
    for (int i = 0; i<64; i++){
        this->game_board[i] = Piece();
    }
    num_pieces = 0;
}
size_t Board::get_moves(std::array<Move, max_legal_moves>& moves, const uint8_t turn_color) const{
    //Iterate over the locations of the pieces. Get all their available moves, add into array and return number of moves.
    return 0;
    
}
bool Board::operator==(const Board& other)const{
    for (int i = 0; i<64;i++) if (!(game_board[i] == other.game_board[i])) return false; 
    if (num_pieces !=other.num_pieces) return false;
    return true;
}

Board::Board(){
    for (int i = 0; i<64; i++){
        game_board[i] = Piece();
    }
}

Board::~Board(){
}


void Board::print_piece_loc() const{
    for(int i =0; i<num_pieces; i++) std::cout << NotationInterface::string_from_idx(piece_locations[i]) << " ";
    std::cout<<std::endl;
}