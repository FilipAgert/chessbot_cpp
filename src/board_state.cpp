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