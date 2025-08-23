#ifndef PIECE_H
#define PIECE_H

#include <cstdint> // for int8_t, uint8_t
#include <string>
#include <iostream>


struct Piece {
protected:
    uint8_t value;
public:
    //Constructor
    constexpr Piece(uint8_t val) : value(val) {}
    constexpr Piece() : value(none) {} // Default constructor initializes to none

    uint8_t get_type() const;
    uint8_t get_color() const;
    char get_char() const;
    void print_val() const {
        std::cout << "Piece value: " << (value) << std::endl;
    }

    static Piece piece_from_char(char c);
    static uint8_t piece_col_from_char(char c);
    static uint8_t piece_type_from_char(char c);
    inline static bool are_colors_equal(const Piece color1, const Piece color2) {
        return (color1.value & color_mask) == (color2.value & color_mask);
    }
    inline static bool are_types_equal(const Piece type1, const Piece type2) {
        return (type1.value & piece_mask) == (type2.value & piece_mask);
    }
    inline static bool is_valid_piece(const Piece p) {
        return (p.value & piece_mask) != none && 
               (p.value & color_mask) != none;
    }
    inline bool operator==(const Piece& other)const {
        return value == other.value;
    }

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

    static const std::string white_pieces;
    static const std::string black_pieces;
    static const std::string all_piece_symbols;
    
    
};
static constexpr Piece none_piece = {0};
#endif
