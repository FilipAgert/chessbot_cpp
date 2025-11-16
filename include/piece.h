#ifndef PIECE_H
#define PIECE_H

#include <cstdint> // for int8_t, uint8_t
#include <string>
#include <iostream>
#include <integer_representation.h>


struct Piece {
protected:
    uint8_t value;
public:
       // Numeric constructor: 0–63 piece code, etc.
    template<
        typename T,
        typename = std::enable_if_t<
            std::is_integral_v<T> &&
            !std::is_same_v<T, char>
        >
    >
    explicit Piece(T v) {
        value = static_cast<uint8_t>(v);
    }

    constexpr Piece() : value(none) {} // Default constructor initializes to none
    /**
     * @brief Creates and returns a piece given a character representing it. error piece if nonvalid character.
     * 
     * @param c 
     * @return Piece 
     */
    explicit Piece(char c);

    /**
     * @brief Get the type of the piece (knight, pawn, etc..)
     * 
     * @return uint8_t value corresponding to type
     */
    uint8_t get_type() const;
    /**
     * @brief Get the color of the piece
     * 
     * @return uint8_t 
     */
    uint8_t get_color() const;
    /**
     * @brief Get the value of the piece. I.e. the uint8_t containing the type and color information
     * 
     * @return constexpr uint8_t 
     */
    inline constexpr uint8_t get_value() const {return value;};
    inline void set_value(uint8_t v){value = v;};
    /**
     * @brief Get the character representing this piece. I.e. a white pawn would be P
     * 
     * @return char 
     */
    char get_char() const;
    void print_val() const {
        std::cout << "Piece value: " << ((int)value) << std::endl;
    }

    /**
     * @brief Returns the character piece symbolising the given uint8_t value. Works on colors.
     * 
     * @param value 
     * @return char 
     */
    static char char_from_val(uint8_t value) {return all_piece_symbols[value];};
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


    static const std::string white_pieces;
    static const std::string black_pieces;
    static const std::string all_piece_symbols;
    
    
};
static constexpr Piece none_piece = Piece();
#endif
