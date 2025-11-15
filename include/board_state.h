#ifndef BOARDSTATE_H
#define BOARDSTATE_H
#include <board.h>
#include <piece.h>
#include <cstdint>
#include <array>
#include <constants.h>
#include <string>
#include <move.h>
#include <exception>
struct BoardState{
    Board board;
    std::array<uint8_t, 32> piece_locations{};//Array containing the indices for pieces in the board array.
    uint8_t num_pieces = err_val8;
    uint8_t castling = err_val8;
    uint8_t turn_color = err_val8; //0b01000 for white 0b10000 for black
    bool en_passant = false;
    uint8_t en_passant_square = err_val8;
    uint8_t check = 0; //0 For no check, white for white checked, black for black checked.
    
    int ply_moves;
    int full_moves;
   /**
    * @brief Does a move. Required: From and to square. Promotion. Changes board state accordingly and stores information into move.
    * 
    * @param move 
    * @return * void 
    */
    void do_move(Move& move);
    void undo_move(Move move);
    /**
     * @brief Changes whose turn it is: white <-> black. Only the turn_color parameter is changed.
     * 
     */
    void change_turn(){turn_color ^= Piece::color_mask;} //Xor with color mask to change color.

    void reset(){
        piece_locations.fill(0);
        num_pieces = 0;
        castling = err_val8;
        turn_color = err_val8;
        en_passant = false;
        en_passant_square = err_val8;
        check = 0;
        ply_moves = err_val8;
        full_moves = err_val8;
        board.clear_board();
    };

    static constexpr uint8_t cast_white_kingside = 0b1;
    static constexpr uint8_t cast_white_queenside = 0b10;
    static constexpr uint8_t cast_black_kingside = 0b100;
    static constexpr uint8_t cast_black_queenside = 0b1000;
    
    /** @brief in the piece_loc array, for the element with "from" as its square, change it to to.
     * 
     * @param from square to be replaced
     * @param to square to be replaced by
     */
    void piece_loc_move(uint8_t from, uint8_t to);
    /**
     * @brief Removes piece from piece_loc array shift all other elements to the left. 
     * 
     * @param sq 
     */
    void piece_loc_remove(uint8_t sq);
};









#endif