#ifndef BOARD_H
#define BOARD_H

#include <constants.h>
#include <cstdint>
#include <piece.h>
#include <string>
#include <iostream>
#include <array>
#include <notation_interface.h>
struct Board {
public:
    Board();
    ~Board();
    Board(std::string fen);
    
 
    inline Piece get_piece_at(uint8_t row, uint8_t col)const{
        return get_piece_at(NotationInterface::idx(row,col));
    };

    inline Piece get_piece_at(uint8_t square)const{return game_board[square];};
    
    void move_piece(uint8_t start_square, uint8_t end_square);

    void add_piece(uint8_t square, Piece Piece);

    void remove_piece(uint8_t square);

    bool is_square_empty(uint8_t square)const;

    uint8_t get_square_color(uint8_t square)const;

    void clear_board();
    
    bool operator==(const Board& other)const;

    

private:
    std::array<Piece, 64> game_board;
};
#endif
