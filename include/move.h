#ifndef MOVE_H
#define MOVE_H
#include <cstdint>
#include <piece.h>
#include <constants.h>
#include <notation_interface.h>
#include <string>
#include <exception>
struct Move{
    Piece captured_piece = none_piece; //captured.
    Piece promotion = none_piece; //Piece to be promoted into
    uint8_t start_square, end_square;
    uint8_t en_passant_square = err_val8;
    uint8_t castling_rights;
    uint8_t check;//This way we don't have to recompute if checked when we undo a move.
    
    Move(std::string move_str){
        if (move_str.length() < 4 || move_str.length() > 5) throw new std::invalid_argument("Move string invalid: " + move_str);
        this->start_square = NotationInterface::idx_from_string(move_str.substr(0,2));
        this->end_square = NotationInterface::idx_from_string(move_str.substr(2,2));
        this->promotion = (move_str.length()==5) ? Piece(move_str[4]) : Piece();
    };
    Move(){};
};


#endif