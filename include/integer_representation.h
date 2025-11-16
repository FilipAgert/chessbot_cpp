#ifndef INTEGER_REP_H
#define INTEGER_REP_H
#include <cstdint>

namespace castling{
    static constexpr uint8_t cast_white_kingside = 0b1;
    static constexpr uint8_t cast_white_queenside = 0b10;
    static constexpr uint8_t cast_black_kingside = 0b100;
    static constexpr uint8_t cast_black_queenside = 0b1000;
};

namespace pieces{
    static constexpr uint8_t none   = 0b000;
    static constexpr uint8_t pawn   = 0b110;
    static constexpr uint8_t bishop = 0b101;
    static constexpr uint8_t knight = 0b100;
    static constexpr uint8_t rook   = 0b011;
    static constexpr uint8_t queen  = 0b010;
    static constexpr uint8_t king   = 0b001;

    static constexpr uint8_t white  = 0b01000; //8
    static constexpr uint8_t black  = 0b10000; //16

    static constexpr uint8_t piece_mask = 0b00111;
    static constexpr uint8_t color_mask = 0b11000;
}
#endif