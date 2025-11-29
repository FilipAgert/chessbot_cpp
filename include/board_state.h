// Copyright 2025 Filip Agert
#ifndef BOARDSTATE_H
#define BOARDSTATE_H
#include <board.h>
#include <constants.h>
#include <integer_representation.h>
#include <move.h>
#include <piece.h>

#include <array>
#include <cstdint>
#include <string>
struct BoardState {
    Board board;
    uint8_t castling = err_val8;
    uint8_t turn_color = err_val8;  // 0b01000 for white 0b10000 for black
    bool en_passant = false;
    uint8_t en_passant_square = err_val8;
    uint8_t check = 0;  // 0 For no check, white for white checked, black for black checked.

    int ply_moves;
    int full_moves;
    /**
     * @brief Does a move. Required: From and to square. Promotion. Changes board state accordingly
     * and stores information into move.
     *
     * @param move
     * @return * void
     */
    void do_move(Move &move);
    void undo_move(const Move move);
    /**
     * @brief Changes whose turn it is: white <-> black. Only the turn_color parameter is changed.
     *
     */
    void change_turn() {
        turn_color ^= pieces::color_mask;
    }  // Xor with color mask to change
       // color.

    /**
     * @brief Get the all the possible legal moves and sets into provided array
     *
     * @param moves array containing moves
     * @return * size_t: number of legal moves in array.
     */
    size_t get_moves(std::array<Move, max_legal_moves> &moves) const;
    void reset() {
        castling = err_val8;
        turn_color = err_val8;
        en_passant = false;
        en_passant_square = err_val8;
        check = 0;
        ply_moves = err_val8;
        full_moves = err_val8;
        board.clear_board();
    };

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
    std::string fen_from_state() const;
    void print_piece_loc() const;
    void Display_board();
    bool operator==(const BoardState &other) const;
    uint8_t get_num_pieces() { return board.get_num_pieces(); }
};

#endif
