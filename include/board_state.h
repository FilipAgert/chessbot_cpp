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
#include <sstream>
#include <integer_representation.h>
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
    void undo_move(const Move move);
    /**
     * @brief Changes whose turn it is: white <-> black. Only the turn_color parameter is changed.
     * 
     */
    void change_turn(){turn_color ^= color_mask;} //Xor with color mask to change color.


    /**
     * @brief Get the all the possible legal moves and sets into provided array
     * 
     * @param moves array containing moves
     * @return * size_t: number of legal moves in array. 
     */
    size_t get_moves(std::array<Move, max_legal_moves>& moves) const;
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

    /**
     * @brief Adds piece to array. Assumes that num_pieces is correct (PRE adding) I.e. if num_pieces is 1, will add to index 1 (position 2) in array.
     * 
     * @param sq Square of piece location
     */
    inline void piece_loc_add(const uint8_t sq) {piece_locations[num_pieces] = sq;};


    /**
     * @brief Takes a FEN string and sets it into the board state.
     * 
     * @param FEN String containing FEN
     * @return true : Successfully parsed FEN.
     * @return false : Did not succesfully parse FEN. BoardState undefined
     */
    bool read_fen(std::string FEN);
    
    /**
     * @brief Outputs fen from state.
     * 
     * @return std::string 
     */
    std::string fen_from_state()const;
    void print_piece_loc() const;
    void Display_board();
    bool operator==(const BoardState& other)const;
};









#endif