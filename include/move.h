#ifndef MOVE_H
#define MOVE_H
#include <cstdint>
#include <piece.h>
#include <constants.h>
struct Move{
    Piece captured_piece = none_piece; //captured.
    Piece promotion = none_piece; //Piece to be promoted into
    uint8_t start_square, end_square;
    uint8_t en_passant_square = err_val8;
    uint8_t castling_rights;
    uint8_t check;//This way we don't have to recompute if checked when we undo a move.
};


#endif