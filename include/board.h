// Copyright 2025 Filip Agert
#ifndef BOARD_H
#define BOARD_H

#include <bitboard.h>
#include <constants.h>
#include <move.h>
#include <notation_interface.h>
#include <piece.h>

#include <array>
#include <cstdint>
#include <string>
struct Board {
 public:
    Board();
    ~Board();
    explicit Board(std::string fen);

    inline Piece get_piece_at(uint8_t row, uint8_t col) const {
        return get_piece_at(NotationInterface::idx(row, col));
    }

    inline Piece get_piece_at(uint8_t square) const { return game_board[square]; }

    void move_piece(const uint8_t start_square, const uint8_t end_square);

    void add_piece(const uint8_t square, const Piece Piece);

    void remove_piece(const uint8_t square);

    void capture_piece(const uint8_t start_square, const uint8_t end_square);
    void capture_piece_ep(const uint8_t start_square, const uint8_t end_square,
                          const uint8_t captured_pawn_loc);

    void promote_piece(const uint8_t square, const Piece promotion);

    bool is_square_empty(uint8_t square) const;

    uint8_t get_square_color(uint8_t square) const;

    void clear_board();

    bool operator==(const Board &other) const;

    /**
     * @brief Get the all possible moves for specified player.
     *
     * @param moves Array containing the moves
     * @param turn_color Color of player to find moves for.
     * @return size_t Number of legal moves found.
     */
    size_t get_moves(std::array<Move, max_legal_moves> &moves, const uint8_t turn_color) const;

    inline uint8_t get_num_pieces() { return num_pieces; }
    void print_piece_loc() const;

 protected:
    std::array<Piece, 64> game_board;
    std::array<uint8_t, 32>
        piece_locations{};  // Array containing the indices for pieces in the board array.
    // Color                 W          B
    // Bitboards: Pieces: [9-14]   [17-22].
    //            Attack: 15         23

    std::array<uint64_t, 32> bit_boards{};

    uint8_t num_pieces = 0;
    /**
     * @brief Adds piece to array. Assumes that num_pieces is correct (PRE adding) I.e. if
     * num_pieces is 1, will add to index 1 (position 2) in array.
     *
     * @param sq Square of piece location
     */
    inline void piece_loc_add(const uint8_t sq) { piece_locations[num_pieces] = sq; }

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
