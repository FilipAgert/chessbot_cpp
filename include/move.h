#ifndef MOVE_H
#define MOVE_H
#include <cstdint>
#include <piece.h>
#include <constants.h>
struct Move{
    Piece captured_piece = none_piece; //captured.
    Piece promotion = none_piece; //Piece to be promoted into
    Piece moved_piece;
    uint8_t start_square, end_square;
    uint8_t en_passant_square = err_val8;
    uint8_t castling_rights;
};


#endif