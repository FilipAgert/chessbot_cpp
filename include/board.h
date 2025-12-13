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
struct restore_move_info {
    uint8_t castleinfo;
    uint8_t ep_square;
    int ply_moves;
    Piece captured;
};
struct Board {
 private:
    uint8_t castleinfo = err_val8;
    uint8_t turn_color = err_val8;  // 0b01000 for white 0b10000 for black
    bool en_passant = false;
    uint8_t en_passant_square = err_val8;
    uint8_t check = 0;  // 0 For no check, white for white checked, black for black checked.

    int ply_moves;
    int full_moves;

 public:
    Board();
    ~Board();
    uint8_t get_turn_color() const { return turn_color; }
    int get_ply_moves() const { return ply_moves; }
    bool get_en_passant() const { return en_passant; }
    uint8_t get_en_passant_square() const { return en_passant_square; }
    uint8_t get_castling() const { return castleinfo; }
    int get_full_moves() const { return full_moves; }
    uint8_t get_check() const { return check; }
    /**
     * @brief Does a move. Required: From and to square. Promotion. Changes board state accordingly
     * and stores information into move.
     *
     * @param move
     * @return * void
     */
    template <bool white_to_move> restore_move_info do_move(Move &move) {
        Piece_t moved = game_board[move.source].get_type();
        switch (moved) {
        case pieces::pawn:
            return do_move<white_to_move, pieces::pawn>(move);
            break;
        case pieces::bishop:
            return do_move<white_to_move, pieces::bishop>(move);
            break;
        case pieces::knight:
            return do_move<white_to_move, pieces::knight>(move);
            break;
        case pieces::rook:
            return do_move<white_to_move, pieces::rook>(move);
            break;
        case pieces::queen:
            return do_move<white_to_move, pieces::queen>(move);
            break;
        case pieces::king:
            return do_move<white_to_move, pieces::king>(move);
            break;
        }
    }
    template <bool white_to_move, Piece_t moved> restore_move_info do_move(Move &move) {
        // Find out move flag:

        if constexpr (moved == pieces::pawn) {
        } else if constexpr (moved == pieces::bishop) {
        } else if constexpr (moved == pieces::knight) {
        } else if constexpr (moved == pieces::rook) {
        } else if constexpr (moved == pieces::queen) {
        } else if constexpr (moved == pieces::king) {
        }
    }
    template <bool white_to_move, Piece_t moved, Flag_t flag> void do_move(Move &move);
    template <bool white_to_move, Piece_t moved, Flag_t flag, Piece_t captured>
    void do_move(Move &move);
    template <bool white_moved> void undo_move(const Move move);
    /**
     * @brief Calculates if king is in check.
     *
     * @param[in] turn_color Color of the king to be check if in check
     * @return True if checked, false if not in check
     */
    template <bool is_white> bool king_checked() const {
        BB king_bb = get_piece_bb<pieces::king, is_white>();
        uint8_t kingsq = BitBoard::lsb(king_bb);
        BB occ = occupancy();
        if ((movegen::knight_atk(kingsq) & get_piece_bb<pieces::knight, !is_white>()) > 0)
            return true;
        if ((movegen::bishop_atk(kingsq, occ) & (get_piece_bb<pieces::bishop, !is_white>() |
                                                 get_piece_bb<pieces::queen, !is_white>())) > 0)
            return true;
        if ((movegen::rook_atk(kingsq, occ) & (get_piece_bb<pieces::rook, !is_white>() |
                                               get_piece_bb<pieces::queen, !is_white>())) > 0)
            return true;
        if ((movegen::pawn_atk_bb<is_white>(king_bb) & get_piece_bb<pieces::pawn, !is_white>()) > 0)
            return true;
        if ((movegen::king_atk(kingsq) & get_piece_bb<pieces::king, !is_white>()) > 0)
            return true;
        return false;
    }
    /**
     * @brief Get the all the possible legal moves and sets into provided array
     *
     * @tparam stype of move to generate.
     * @param moves array containing moves
     * @return * size_t: number of legal moves in array.
     */
    template <search_type stype, bool is_white>
    size_t get_moves(std::array<Move, max_legal_moves> &moves) {
        std::array<Move, max_legal_moves> pseudolegal_moves;
        size_t num_pseudolegal_moves = get_pseudolegal_moves<stype, is_white>(pseudolegal_moves);
        //  uint8_t opposite_color = turn_color ^ color_mask;

        size_t num_moves = 0;
        for (size_t m = 0; m < num_pseudolegal_moves;
             m++) {  // For movechecker, we can have cheaper do_move
                     // undo_move. Dont need to handle everything.
                     // E.g. board could have a bitboard version only.
                     // PERF: Implement method in Board that only does/undoes moves in the bitboards
                     // for performance.
            do_move(pseudolegal_moves[m]);
            if (!king_checked<is_white>()) {
                // PERF: Check how expensive this king_checked thing is. Is it worth the move
                // ordering benefit?
                // bool opponent_checked = board.king_checked(opposite_color);
                // pseudolegal_moves[m].check = opponent_checked;
                moves[num_moves++] = pseudolegal_moves[m];
            }
            undo_move(pseudolegal_moves[m]);
        }
        return num_moves;
    }
    /**
     * @brief Get the all possible moves for specified player.
     *
     * @tparam stype of moves to generate
     * @param[out] Array containing the moves
     * @param[in] Color of player to find moves for.
     * @param[in] en_passant bool flag for en passant
     * @param[in] en_passant_sq square index containing en_passant square
     * @param[in] castle_info integer containing information for castling.
     * @return size_t Number of legal moves found.
     */
    template <search_type stype, bool is_white>
    size_t get_pseudolegal_moves(std::array<Move, max_legal_moves> &moves) const {
        size_t num_moves = 0;
        BB friendly_bb = occupancy<is_white>();
        BB enemy_bb = occupancy<!is_white>();
        BB queen_bb = get_piece_bb<pieces::queen, is_white>();
        BB bishop_bb = get_piece_bb<pieces::bishop, is_white>();
        BB rook_bb = get_piece_bb<pieces::rook, is_white>();
        BB pawn_bb = get_piece_bb<pieces::pawn, is_white>();
        BB king_bb = get_piece_bb<pieces::king, is_white>();
        BB knight_bb = get_piece_bb<pieces::knight, is_white>();
        uint64_t ep_bb = en_passant ? BitBoard::one_high(en_passant_square) : 0;
        gen_add_all_moves<pieces::queen, stype, is_white>(moves, num_moves, queen_bb, friendly_bb,
                                                          enemy_bb, ep_bb, castleinfo);
        gen_add_all_moves<pieces::bishop, stype, is_white>(moves, num_moves, bishop_bb, friendly_bb,
                                                           enemy_bb, ep_bb, castleinfo);
        gen_add_all_moves<pieces::rook, stype, is_white>(moves, num_moves, rook_bb, friendly_bb,
                                                         enemy_bb, ep_bb, castleinfo);
        gen_add_all_moves<pieces::king, stype, is_white>(moves, num_moves, king_bb, friendly_bb,
                                                         enemy_bb, ep_bb, castleinfo);
        gen_add_all_moves<pieces::knight, stype, is_white>(moves, num_moves, knight_bb, friendly_bb,
                                                           enemy_bb, ep_bb, castleinfo);
        gen_add_all_moves<pieces::pawn, stype, is_white>(moves, num_moves, pawn_bb, friendly_bb,
                                                         enemy_bb, ep_bb, castleinfo);
        return num_moves;
    }
    /**
     * @brief Changes whose turn it is: white <-> black. Only the turn_color parameter is changed.
     *
     */
    void change_turn() {
        turn_color ^= pieces::color_mask;
    }  // Xor with color mask to change
       // color.

    void reset() {
        castleinfo = err_val8;
        turn_color = err_val8;
        en_passant = false;
        en_passant_square = err_val8;
        check = 0;
        ply_moves = err_val8;
        full_moves = err_val8;
        clear_board();
    }

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
    void Display_board();
    bool operator==(const Board &other) const;
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
    /**
     * @brief Gets number of pieces for a player
     *
     * @tparam is_white white or black
     * @return number of pieces for selected player
     */
    template <bool is_white> inline constexpr uint8_t get_num_pieces() {
        return BitBoard::bitcount(occupancy<is_white>());
    }
    void print_piece_loc() const;

    /**
     * @brief Gets the attack bitboard for a given color. These are squares that are threatened by
     * this player.
     *
     * @param[in] color Color of player you want to get atk bitboard for
     * @return [ALl squares under attack by given player.]
     */
    template <bool for_white> uint64_t get_atk_bb() const {
        BB friendly_pieces = occupancy<for_white>();
        BB enemy_pieces = occupancy<!for_white>();
        BB occ = friendly_pieces | enemy_pieces;
        BB queen_bb = get_piece_bb<pieces::queen, for_white>();
        BB bishop_bb = get_piece_bb<pieces::bishop, for_white>();
        BB rook_bb = get_piece_bb<pieces::rook, for_white>();
        BB pawn_bb = get_piece_bb<pieces::pawn, for_white>();
        BB king_bb = get_piece_bb<pieces::king, for_white>();
        BB knight_bb = get_piece_bb<pieces::knight, for_white>();

        bishop_bb = movegen::bishop_atk_bb(bishop_bb | queen_bb, occ);
        rook_bb = movegen::rook_atk_bb(rook_bb | queen_bb, occ);
        knight_bb = movegen::knight_atk_bb(knight_bb);
        pawn_bb = movegen::pawn_atk_bb<for_white>(pawn_bb);
        king_bb = movegen::king_atk_bb(king_bb);
        return bishop_bb | rook_bb | knight_bb | pawn_bb | king_bb;
    }

    /**
     * @brief Gets piece bitboard compile time.
     *
     * @tparam pval [Value of piece]
     * @tparam is_white get white or black piece
     * @return Bitboard of piece
     */
    template <Piece_t pval, bool is_white> BB get_piece_bb() const {
        constexpr uint8_t type = pval & pieces::piece_mask;
        if constexpr (is_white) {
            if constexpr (type == pieces::pawn)
                return white_pawns;
            else if constexpr (type == pieces::knight)
                return white_knights;
            else if constexpr (type == pieces::rook)
                return white_rooks;
            else if constexpr (type == pieces::bishop)
                return white_bishops;
            else if constexpr (type == pieces::queen)
                return white_queen;
            else if constexpr (type == pieces::king)
                return white_king;
        } else {
            if constexpr (type == pieces::pawn)
                return black_pawns;
            else if constexpr (type == pieces::knight)
                return black_knights;
            else if constexpr (type == pieces::rook)
                return black_rooks;
            else if constexpr (type == pieces::bishop)
                return black_bishops;
            else if constexpr (type == pieces::queen)
                return black_queen;
            else if constexpr (type == pieces::king)
                return black_king;
        }
    }
    /**
     * @brief Gets piece bitboard compile time.
     * @tparam pval [Value of piece]
     * @return Bitboard of piece
     */
    template <Piece_t pval> BB get_piece_bb(uint8_t col) const {
        if (col == pieces::white)
            return get_piece_bb<pval, true>();
        else
            return get_piece_bb<pval, false>();
    }
    /**
     * @brief Gets number of pieces of a type and color on the board.
     *
     * @tparam Piece_t piece type
     * @tparam is_white flag if white or not
     * @return Number of pieces
     */
    template <Piece_t piece, bool is_white> uint8_t get_piece_cnt() const {
        return BitBoard::bitcount(get_piece_bb<piece, is_white>());
    }

    /**
     * @brief Gets a vector of the pieces on the board.
     *
     */
    std::vector<Piece> get_pieces();

    BB occupancy() const { return occupancy<true>() | occupancy<false>(); }
    template <bool is_white> BB occupancy() const {
        if constexpr (is_white)
            return white_pieces;
        else
            return black_pieces;
    }
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
    constexpr int get_piece_mobility() const {  // TODO: Add EP squares, add castling.
        BB piece_bb = get_piece_bb<pval, is_white>();

        int mobility = 0;
        BB occ = occupancy();
        BB friendly = occupancy<is_white>();
        if constexpr (omit_pawn_controlled) {
            BB enemy_pawn_bb = get_piece_bb<pieces::pawn, !is_white>();
            BB enemy_pawn_atk = movegen::pawn_atk_bb<!is_white>(enemy_pawn_bb);
            BitLoop(piece_bb) {
                uint8_t sq = BitBoard::lsb(piece_bb);
                BB piece_atk_bb =
                    movegen::get_atk_bb<pval, is_white>(sq, occ) & ~(friendly | enemy_pawn_atk);
                mobility += BitBoard::bitcount(piece_atk_bb);
            }
        } else {
            BitLoop(piece_bb) {
                uint8_t sq = BitBoard::lsb(piece_bb);
                BB piece_atk_bb = movegen::get_atk_bb<pval, is_white>(sq, occ) & ~friendly;
                mobility += BitBoard::bitcount(piece_atk_bb);
            }
        }
        return mobility;
    }

 protected:
    template <bool is_white> constexpr uint8_t get_pawn_promote_rank() {
        if constexpr (is_white)
            return 7;
        else
            return 0;
    }
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
    template <bool is_white, Piece_t type>
    void add_moves(std::array<Move, max_legal_moves> &moves, size_t &num_moves, uint64_t &to_bb,
                   const uint8_t from) const {
        constexpr uint8_t friendly_longsq = is_white ? 0 : 56;
        constexpr uint8_t friendly_shortsq = is_white ? 7 : 63;
        constexpr uint8_t enemy_longsq = is_white ? 56 : 0;
        constexpr uint8_t enemy_shortsq = is_white ? 63 : 7;
        BitLoop(to_bb) {
            uint8_t lsb = BitBoard::lsb(to_bb);
            Flag_t flag = moveflag::MOVEFLAG_silent;
            if constexpr (type == pieces::pawn) {
                if (NotationInterface::row(lsb) == get_pawn_romote_rank<is_white>()) {
                    constexpr uint8_t col = is_white ? pieces::white : pieces::black;
                    for (uint8_t ptype : pieces::promote_types)
                        moves[num_moves++] = Move(from, lsb, Piece(ptype | col));
                    // FIX: promotions cant both remove castle and promote flag.

                    // flag set automatically in move constructor.
                    continue;
                } else {
                    if (lsb == en_passant_square) {
                        flag = moveflag::MOVEFLAG_pawn_ep_capture;
                    } else if (abs(static_cast<int>(from) - static_cast<int>(lsb)) == 16) {
                        flag = moveflag::MOVEFLAG_pawn_double_push;
                    }
                }
            } else if constexpr (type == pieces::king) {
                if (lsb - from == 2) {
                    flag = moveflag::MOVEFLAG_short_castling;
                } else if (from - flag == 2) {
                    flag = moveflag::MOVEFLAG_long_castling;
                } else {
                    flag = moveflag::MOVEFLAG_remove_all_castle;
                }
            } else if constexpr (type == pieces::rook) {
                if (from == friendly_longsq)
                    flag = moveflag::MOVEFLAG_remove_long_castle;
                else if (from == friendly_shortsq)
                    flag = moveflag::MOVEFLAG_remove_short_castle;
            }
            if (lsb == enemy_longsq)
                flag = moveflag::MOVEFLAG_remove_long_castle;
            else if (lsb == enemy_shortsq)
                flag = moveflag::MOVEFLAG_remove_short_castle;
            moves[num_moves++] = Move(from, lsb, flag);
        }
    }
}

/**
 * @brief For a bitboard with locations of pieces, generate all possible
 * moves and add to move vector
 *
 * @tparam[in] ptype type of piece.
 * @tparam[in] stype type of search.
 * @param[inout] moves array containing moves generated so far. will be
 * filled with new moves
 * @param[inout] num_moves number of moves generated before this
 * subroutine on in, increased by number of moves this routine generated
 * on oute
 * @param[in] piece_bb bitboard with piece locations. destroyed by this
 * method
 * @param[in] friendly_bb bb with all friendly pieces
 * @param[in] enemy_bb bb with all enemy pieces
 * @param[in] ep_bb bitboard with en passant square
 * @param[in] castleinfo int containing info about a castle
 * @param[in] turn_color color of player
 */
template <Piece_t ptype, search_type stype, bool is_white>
void gen_add_all_moves(std::array<Move, max_legal_moves> &moves, size_t &num_moves,
                       uint64_t &piece_bb, const uint64_t friendly_bb, const uint64_t enemy_bb,
                       const uint64_t ep_bb, const uint8_t castleinfo) const {
    BitLoop(piece_bb) {
        uint8_t sq = BitBoard::lsb(piece_bb);
        uint64_t to_sqs =
            to_squares<ptype, stype, is_white>(sq, friendly_bb, enemy_bb, ep_bb, castleinfo);
        add_moves<is_white, ptype>(moves, num_moves, to_sqs, sq);
    }
}
}

std::array<Piece, 64> game_board;
// Color                 W          B
// Bitboards: Pieces: [9-14]   [17-22].
//            Attack: 15         23
//            All p : 8          16

std::array<uint64_t, 32> bit_boards{};
BB white_rooks, white_pawns, white_knights, white_bishops, white_queen, white_king, white_pieces;
BB black_rooks, black_pawns, black_knights, black_bishops, black_queen, black_king, black_pieces;

uint8_t num_pieces = 0;

/**
 * @brief For a given piece type, generate all possible to squares.
 *
 * @tparam[in] ptype Piece type
 * @tparam[in] s_type type of search to be conducted.
 * @param[in] sq Square of piece
 * @param[in] friendly_bb Bit board of all friendly pieces
 * @param[in] enemy_bb Bit board of all enemy pieces
 * @param[in] ep_bb En passant bit board
 * @param[in] castleinfo Integer containing castle information
 * @param[in] turn_color Color of player to eval
 * @return bitboard containing ones in the squares where this piece (or
 * pieces) can move to.
 */
template <Piece_t ptype, search_type s_type, bool is_white>
BB to_squares(uint8_t sq, BB friendly_bb, BB enemy_bb, BB ep_bb, uint8_t castleinfo) const {
    BB piece_bb = BitBoard::one_high(sq);
    constexpr uint8_t color = is_white ? pieces::white : pieces::black;
    BB to_squares;
    if constexpr (ptype == pieces::pawn) {
        to_squares = movegen::pawn_moves(piece_bb, friendly_bb, enemy_bb, ep_bb, color);
    } else if constexpr (ptype == pieces::bishop) {
        to_squares = movegen::bishop_moves_sq(sq, friendly_bb, enemy_bb);
    } else if constexpr (ptype == pieces::knight) {
        to_squares = movegen::knight_moves(sq, friendly_bb);
    } else if constexpr (ptype == pieces::rook) {
        to_squares = movegen::rook_moves_sq(sq, friendly_bb, enemy_bb);
    } else if constexpr (ptype == pieces::queen) {
        to_squares = movegen::queen_moves_sq(sq, friendly_bb, enemy_bb);
    } else if constexpr (ptype == pieces::king) {
        to_squares = movegen::king_moves(sq, friendly_bb, friendly_bb | enemy_bb,
                                         get_atk_bb<!is_white>(), castleinfo, color);
    }
    if constexpr (s_type.quiesence_search) {  // Only search for captures
                                              // in Quiesence.
        to_squares &= enemy_bb;
    }
    return to_squares;
}
/**
 * @brief Handles moving piece on bitboard.
 * board.
 *
 * @param[in] from index of from square
 * @param[in] to index of to square
 */
template <Piece_t piece, bool is_white> void bb_move(const uint8_t from, const uint8_t to);
/**
 * @brief Handles removing piece on bitboard. WARNING: Must be called
 * before removing piece on board.
 *
 * @param[in] sq index of square to remove piece from
 * @param[in] piece to add
 */
template <Piece_t piece, bool is_white> void bb_remove(const uint8_t sq);
/**
 * @brief Handles moving piece on bitboard.
 * board.
 *
 * @param[in] from index of from square
 * @param[in] piece to add
 */
template <Piece_t piece, bool is_white> void bb_add(const uint8_t sq);
}
;
#endif
