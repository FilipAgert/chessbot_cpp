#ifndef BOARD_H
#define BOARD_H

#include <constants.h>
#include <cstdint>
#include <piece.h>
#include <string>
#include <iostream>
#include <array>
struct Board {
public:
    Board();
    ~Board();
    Board(std::string fen);
    
    /**
    * @brief Gets linear idx from row and col
    * @brief row (0-7)
    * @brief col (0-7)
    * @return linear idx [0-63
    */
    inline static uint8_t idx(uint8_t row, uint8_t col){
        return col+row*8;
    };

    /**
     * @brief Gets row, col from idx
     * 
     * @param row Out: Row [0-7]
     * @param col Out: Col [0-7]
     * @param idx Square idx [0-63]
     */
    static void row_col(uint8_t & row, uint8_t & col, const uint8_t idx);
    /**
    * @brief Gets index from square string e.g. "A5"
    */
    static uint8_t idx_from_string(std::string square);
    
    /**
     * @brief Gets algebraic notation from square.
     * 
     * @param idx square idx [0-63]
     * @return std::string E.g. a5
     */
    static std::string string_from_idx(const uint8_t idx);
 
    inline Piece get_piece_at(uint8_t row, uint8_t col)const{
        return get_piece_at(idx(row,col));
    };

    inline Piece get_piece_at(uint8_t square)const{return game_board[square];};
    
    void move_piece(uint8_t start_square, uint8_t end_square);

    void add_piece(uint8_t square, Piece Piece);

    void remove_piece(uint8_t square);

    bool is_square_empty(uint8_t square)const;

    uint8_t get_square_color(uint8_t square)const;

    void clear_board();

private:
    std::array<Piece, 64> game_board;
};
#endif
