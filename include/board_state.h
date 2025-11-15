#ifndef BOARDSTATE_H
#define BOARDSTATE_H
#include <board.h>
#include <piece.h>
#include <cstdint>
#include <array>
#include <constants.h>
#include <string>
struct BoardState{
    Board board;
    std::array<uint8_t, 32> piece_locations{};
    uint8_t num_pieces = err_val8;
    uint8_t castling = err_val8;
    uint8_t turn_color = err_val8;
    bool en_passant = false;
    uint8_t en_passant_square = err_val8;
    uint8_t check = 0; //0 For no check, white for white checked, black for black checked.
    
    int ply_moves;
    int full_moves;

    static constexpr uint8_t cast_white_kingside = 0b1;
    static constexpr uint8_t cast_white_queenside = 0b10;
    static constexpr uint8_t cast_black_kingside = 0b100;
    static constexpr uint8_t cast_black_queenside = 0b1000;
};








#endif