// Copyright 2025 Filip Agert
#ifndef BOARD_H
#define BOARD_H

#include <bitboard.h>
#include <constants.h>
#include <move.h>
#include <notation_interface.h>
#include <piece.h>
#include <vector>

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
     * @param[out] Array containing the moves
     * @param[in] Color of player to find moves for.
     * @param[in] en_passant bool flag for en passant
     * @param[in] en_passant_sq square index containing en_passant square
     * @param[in] castle_info integer containing information for castling.
     * @return size_t Number of legal moves found.
     */
    size_t get_pseudolegal_moves(std::array<Move, max_legal_moves> &moves, const uint8_t turn_color,
                                 const bool en_passant, const uint8_t en_passant_sq,
                                 uint8_t castle_info) const;

    /**
     * @brief Calculates if king is in check.
     *
     * @param[in] turn_color Color of the king to be check if in check
     * @return True if checked, false if not in check
     */
    bool king_checked(const uint8_t turn_color) const;

    inline uint8_t get_num_pieces() { return num_pieces; }
    void print_piece_loc() const;
    inline const uint64_t get_bb(const uint8_t bb_idx) { return bit_boards[bb_idx]; }

    /**
     * @brief Gets the attack bitboard for a given color. These are squares that are threatened by
     * this player.
     *
     * @param[in] color Color of player you want to get atk bitboard for
     * @return [ALl squares under attack by given player.]
     */
    uint64_t get_atk_bb(const uint8_t color) const;

    /**
     * @brief Gets the number of pieces on the board of a certain type (type + color)
     *
     * @param[[TODO:direction]] p [TODO:description]
     * @return [TODO:description]
     */
    size_t get_piece_cnt(Piece p);

    /**
     * @brief Gets a vector of the pieces on the board.
     *
     */
    std::vector<Piece> get_pieces();

    std::vector<std::pair<Piece, uint8_t>> get_piece_num_moves(uint8_t castleinfo, uint64_t ep_bb);

 protected:
    std::array<Piece, 64> game_board;
    std::array<uint8_t, 32>
        piece_locations{};  // Array containing the indices for pieces in the board array.
    // Color                 W          B
    // Bitboards: Pieces: [9-14]   [17-22].
    //            Attack: 15         23
    //            All p : 8          16

    std::array<uint64_t, 32> bit_boards{};

    uint8_t num_pieces = 0;
    /**
     * @brief Adds piece to array. Assumes that num_pieces is correct (PRE adding) I.e. if
     * num_pieces is 1, will add to index 1 (position 2) in array.
     *
     * @param sq Square of piece location
     */
    inline void piece_loc_add(const uint8_t sq) { piece_locations[num_pieces] = sq; }

    /**
     * @brief For a given piece type, generate all possible to squares.
     *
     * @param[in] ptype Piece type
     * @param[in] sq Square of piece
     * @param[in] friendly_bb Bit board of all friendly pieces
     * @param[in] enemy_bb Bit board of all enemy pieces
     * @param[in] ep_bb En passant bit board
     * @param[in] castleinfo Integer containing castle information
     * @param[in] turn_color Color of player to eval
     * @return bitboard containing ones in the squares where this piece (or pieces) can move to.
     */
    uint64_t to_squares(uint8_t ptype, uint8_t sq, uint64_t friendly_bb, uint64_t enemy_bb,
                        uint64_t ep_bb, uint8_t castleinfo, uint8_t turn_color) const;
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
     * @brief Handles moving piece on bitboard.
     * board.
     *
     * @param[in] from index of from square
     * @param[in] to index of to square
     */
    void bb_move(const uint8_t from, const uint8_t to, const Piece p);
    /**
     * @brief Handles removing piece on bitboard. WARNING: Must be called before removing piece on
     * board.
     *
     * @param[in] sq index of square to remove piece from
     * @param[in] piece to add
     */
    void bb_remove(const uint8_t sq, const Piece p);
    /**
     * @brief Handles moving piece on bitboard.
     * board.
     *
     * @param[in] from index of from square
     * @param[in] piece to add
     */
    void bb_add(const uint8_t sq, const Piece p);
};
#endif
