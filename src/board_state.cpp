#include <board_state.h>


void BoardState::do_move(Move& move){
    move.castling_rights = this->castling;
    move.en_passant_square = this-> en_passant ? this->en_passant_square : err_val8; 
    move.captured_piece = this->board.get_piece_at(move.end_square);
    move.moved_piece = this->board.get_piece_at(move.start_square);
    
    ply_moves += 1;
    if(this->turn_color == Piece::black) full_moves += 1;
    change_turn(); //Changes turn color from white <-> black.
    
    if(move.captured_piece.get_value()){
        num_pieces--;
        if(en_passant && move.moved_piece.get_type() == Piece::pawn && move.end_square == en_passant_square){
            std::cerr << "Warning: en passant not implemented in BoardState::do_move";
        }
        //If a piece is removed, we can reuse target piece square since it already points to a square with a piece. 
        //Not valid for en_passant capture.
        //Only need to remove start_square.
        piece_loc_remove(move.start_square);
    } else piece_loc_move(move.start_square, move.end_square);

    board.move_piece(move.start_square, move.end_square);
    if(move.promotion.get_value()) {
        board.add_piece(move.end_square, move.promotion); //Replace pawn by promoted.
        if(move.promotion.get_color() == Piece::none) throw new std::invalid_argument("Promoted piece must have a color");
    }
}

void BoardState::undo_move(const Move move){
    castling = move.castling_rights;
    en_passant_square = move.en_passant_square;
    en_passant = en_passant_square < 64;
    
    board.move_piece(move.end_square, move.start_square);
    if(move.promotion.get_value()){
        board.add_piece(move.start_square,(move.promotion.get_color() | Piece::pawn)); //Replace with pawn.
    };


    if(move.captured_piece.get_value()){
        piece_loc_add(move.start_square); //Counterintuative, but the target square will already track the target piece.
        if(en_passant && move.moved_piece.get_type() == Piece::pawn && move.end_square == en_passant_square){
            std::cerr << "Warning: en passant not implemented in BoardState::undo_move";
        }
        board.add_piece(move.end_square, move.captured_piece);
    } else{
        piece_loc_move(move.end_square, move.start_square);
    } 
   
    
    ply_moves -= 1;
    if(this->turn_color == Piece::white) full_moves -= 1;
    change_turn(); //Changes turn color from white <-> black.
}

void BoardState::piece_loc_remove(uint8_t sq){
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

void BoardState::piece_loc_move(uint8_t from, uint8_t to){
    uint8_t idx = 0;
    while (idx < num_pieces){
        if(piece_locations[idx] == from){
            piece_locations[idx] = to;
            return;
        }
        idx++;
    }
    throw new std::runtime_error("No piece found at square: " + (int)from);
}

bool BoardState::operator==(const BoardState& other)const{
    if (!(board == other.board)) return false;
    if (num_pieces != other.num_pieces) return false;
    if (castling != other.castling) return false;
    if (turn_color != other.turn_color) return false;
    if (en_passant != other.en_passant) return false;
    if (en_passant && (en_passant_square != other.en_passant_square)) return false;
    if (check != other.check) return false;
    if (ply_moves != other.ply_moves) return false;
    if (full_moves != other.full_moves) return false;
    return true;
}