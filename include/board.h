// Copyright 2025 Filip Agert
#ifndef BOARD_H
#define BOARD_H

#include "movegen.h"
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

    void move_piece(const uint8_t source, const uint8_t target);

    void add_piece(const uint8_t square, const Piece Piece);

    void remove_piece(const uint8_t square);

    void capture_piece(const uint8_t source, const uint8_t target);
    void capture_piece_ep(const uint8_t source, const uint8_t target,
                          const uint8_t captured_pawn_loc);

    void promote_piece(const uint8_t square, const Piece promotion);

    bool is_square_empty(uint8_t square) const;

    uint8_t get_square_color(uint8_t square) const;

    void clear_board();

    bool operator==(const Board &other) const;

    /**
     * @brief Calculates if king is in check.
     *
     * @param[in] turn_color Color of the king to be check if in check
     * @return True if checked, false if not in check
     */
    bool king_checked(const uint8_t turn_color) const;

    /**
     * @brief Computes if a move acting on this board state would leave the king of a certain color
     * in check.
     *
     * @param[in] candidate candidate move to check if it leaves the king in check
     * @param[in] king_color color of king to check if is in check
     * @return boolean if king is in check or not
     */
    bool does_move_check(const Move candidate, const uint8_t king_color);
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

    // Piece loc independent way
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
    size_t get_pseudolegal_moves(std::array<Move, max_legal_moves> &moves, const uint8_t color,
                                 const bool en_passant, const uint8_t en_passant_sq,
                                 const uint8_t castleinfo) const;

    /**
     * @brief Gets piece bitboard compile time.
     *
     * @tparam pval [Value of piece]
     * @tparam is_white get white or black piece
     * @return Bitboard of piece
     */
    template <Piece_t pval, bool is_white> BB get_piece_bb() {
        constexpr uint8_t col = is_white ? pieces::white : pieces::black;
        constexpr uint8_t type = pval & pieces::piece_mask;

        return bit_boards[col | type];
    }
    BB occupancy() { return bit_boards[pieces::white] | bit_boards[pieces::black]; }
    /**
     * @brief Gets piece mobility for a given piece type
     *
     * @tparam Piece_t piece type
     * @tparam omit_pawn_controlled flag if we should omit squares controlled by enemy pawns from
     * the mobility count
     * @tparam is_white flag if piece to check is white or not
     * @return number of squares piece of this type and color can move to.
     */
    template <Piece_t pval, bool omit_pawn_controlled, bool is_white>
    constexpr int get_piece_mobility() {  // TODO: Add EP squares, add castling.
        BB piece_bb = get_piece_bb<pval, is_white>();

        int mobility = 0;
        BB occ = occupancy();
        if constexpr (omit_pawn_controlled) {
            BB enemy_pawn_bb = get_piece_bb<pieces::pawn, !is_white>();
            BB enemy_pawn_atk = movegen::pawn_atk_bb(enemy_pawn_bb, !is_white);
            BitLoop(piece_bb) {
                uint8_t sq = BitBoard::lsb(piece_bb);
                BB piece_atk_bb = movegen::get_atk_bb<pval, is_white>(sq, occ) & ~enemy_pawn_atk;
                mobility += BitBoard::bitcount(piece_atk_bb);
            }
        } else {
            BitLoop(piece_bb) {
                uint8_t sq = BitBoard::lsb(piece_bb);
                BB piece_atk_bb = movegen::get_atk_bb<pval, is_white>(sq, occ);
                mobility += BitBoard::bitcount(piece_atk_bb);
            }
        }
        return mobility;
    }

 protected:
    /**
     * @brief From a bitboard of attacked squares, generate all moves and add to array
     *
     * @param[inout] moves array of moves to be added to
     * @param[inout] num_moves number of moves before routine on in, number of moves after routine
     * on out
     * @param[in] to_bb bitboard containing attacking squares. will be destroyed by calling this
     * routine
     * @param[in] from from square.
     */
    void add_moves(std::array<Move, max_legal_moves> &moves, size_t &num_moves, uint64_t &to_bb,
                   const uint8_t from) const;
    /**
     * @brief From a bitboard of attacked squares, generate all moves and add to array. Special case
     * for pawns due to promotions.
     *
     * @param[inout] moves array of moves to be added to
     * @param[inout] num_moves number of moves before routine on in, number of moves after routine
     * on out
     * @param[in] to_bb bitboard containing attacking squares. will be destroyed by calling this
     * routine
     * @param[in] from from square.
     * @param[in] color of pawn.
     */
    void add_moves_pawn(std::array<Move, max_legal_moves> &moves, size_t &num_moves,
                        uint64_t &to_bb, const uint8_t from, const uint8_t color) const;

    /**
     * @brief For a bitboard with locations of pieces, generate all possible moves and add to move
     * vector
     *
     * @param[inout] moves array containing moves generated so far. will be filled with new moves
     * @param[inout] num_moves number of moves generated before this subroutine on in, increased by
     * number of moves this routine generated on oute
     * @param[in] piece_bb bitboard with piece locations. destroyed by this method
     * @param[in] piecetype type of piece
     * @param[in] friendly_bb bb with all friendly pieces
     * @param[in] enemy_bb bb with all enemy pieces
     * @param[in] ep_bb bitboard with en passant square
     * @param[in] castleinfo int containing info about a castle
     * @param[in] turn_color color of player
     */
    void gen_add_all_moves(std::array<Move, max_legal_moves> &moves, size_t &num_moves,
                           uint64_t &piece_bb, const uint8_t piecetype, const uint64_t friendly_bb,
                           const uint64_t enemy_bb, const uint64_t ep_bb, const uint8_t castleinfo,
                           const uint8_t turn_color) const;

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
    /**
     * @brief Handles moving piece on bitboard.
     * board.
     *
     * @param[in] from index of from square
     * @param[in] to index of to square
     */
    void bb_move(const uint8_t from, const uint8_t to, const Piece p);
    /**
     * @brief Handles removing piece on bitboard. WARNING: Must be called before removing piece
     * on board.
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
