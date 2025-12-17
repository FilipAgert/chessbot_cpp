// Copyright 2025 Filip Agert
#ifndef BOARD_H
#define BOARD_H

#include "movegen.h"
#include <bitboard.h>
#include <cassert>
#include <constants.h>
#include <move.h>
#include <notation_interface.h>
#include <piece.h>
#include <stdexcept>
#include <vector>

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
struct restore_move_info {
    uint8_t castleinfo : 4 = 0;
    Piece_t captured : 3 = 0;
    uint8_t ep_square : 6 = 0;
    uint8_t ply_moves = 0;
};
struct Board {
 private:
    uint8_t castleinfo = err_val8;
    uint8_t turn_color = err_val8;  // 0b01000 for white 0b10000 for black
    bool en_passant = false;
    uint8_t en_passant_square = 0;
    uint8_t check = 0;  // 0 For no check, white for white checked, black for black checked.

    uint8_t ply_moves;
    int full_moves;
    std::array<Piece, 64> game_board;
    // Color                 W          B
    // Bitboards: Pieces: [9-14]   [17-22].
    //            Attack: 15         23
    //            All p : 8          16

    BB white_rooks, white_pawns, white_knights, white_bishops, white_queen, white_king, white_pieces;
    BB black_rooks, black_pawns, black_knights, black_bishops, black_queen, black_king, black_pieces;

 public:
    Board();
    ~Board();
    uint8_t get_turn_color() const { return turn_color; }
    uint8_t get_ply_moves() const { return ply_moves; }
    bool get_en_passant() const { return en_passant; }
    uint8_t get_en_passant_square() const { return en_passant_square; }
    uint8_t get_castling() const { return castleinfo; }
    int get_full_moves() const { return full_moves; }
    uint8_t get_check() const { return check; }
    bool board_BB_match();
    /**
     * @brief Does a move. Required: From and to square. Promotion. Changes board state accordingly
     * and stores information into move.
     *
     * @param move
     * @return * void
     */
    template <bool is_white> void add_piece(const uint8_t square, Piece p) {
        switch (p.get_type()) {
        case (pieces::pawn):
            add_piece<is_white, pieces::pawn>(square);
            break;
        case (pieces::knight):
            add_piece<is_white, pieces::knight>(square);
            break;
        case (pieces::rook):
            add_piece<is_white, pieces::rook>(square);
            break;
        case (pieces::queen):
            add_piece<is_white, pieces::queen>(square);
            break;
        case (pieces::king):
            add_piece<is_white, pieces::king>(square);
            break;
        case (pieces::bishop):
            add_piece<is_white, pieces::bishop>(square);
            break;
        default:
            throw std::runtime_error("Error, trying to add invalid piece");
            break;
        }
    }

    template <bool is_white, Piece_t type> inline constexpr void remove_piece(const uint8_t square) {
        bb_remove<is_white, type>(square);
        game_board[square] = none_piece;
    }
    template <bool is_white, Piece_t type> inline constexpr void move_piece(const uint8_t source, const uint8_t target) {
        bb_move<is_white, type>(source, target);
        game_board[target] = game_board[source];
        game_board[source] = none_piece;
    }

    template <bool is_white, Piece_t type> constexpr void add_piece(const uint8_t square) {
        if constexpr (is_white) {
            game_board[square] = Piece(pieces::white | type);
        } else {
            game_board[square] = Piece(pieces::black | type);
        }
        bb_add<is_white, type>(square);
    }
    /**
     * @brief Use this if moveflag not defined yet.
     * SHOULD NOT BE USED BY ENGINE. ONLY BY THE UCI INTERFACE.
     *
     * @tparam white_to_move white to move
     * @param[in] move move to flag
     * @return restore_move_info
     */
    template <bool white_to_move> restore_move_info do_move_no_flag(Move &move) {
        assert(move.is_valid());
        Piece_t moved = get_piece_at(move.source).get_type();
        assert(moved != pieces::none);
        switch (moved) {
        case pieces::pawn:
            // Check for ep...
            if (move.target == en_passant_square && en_passant_square != 0)
                move.flag = moveflag::MOVEFLAG_pawn_ep_capture;

            if constexpr (white_to_move) {
                if (move.target - move.source == 16)
                    move.flag = moveflag::MOVEFLAG_pawn_double_push;
            } else {
                if (move.source - move.target == 16)
                    move.flag = moveflag::MOVEFLAG_pawn_double_push;
            }
            break;
        case pieces::king:
            if (static_cast<int>(move.source) - static_cast<int>(move.target) == 2) {
                // Positive two means queenside / long castle
                move.flag = moveflag::MOVEFLAG_long_castling;
            } else if (static_cast<int>(move.target) - static_cast<int>(move.source) == 2) {
                move.flag = moveflag::MOVEFLAG_short_castling;
            } else if (move.source == get_castle_from_sq<white_to_move, pieces::king, moveflag::MOVEFLAG_long_castling>()) {
                move.flag = moveflag::MOVEFLAG_remove_all_castle;
            }
            break;
        case pieces::rook:
            if (move.source == get_castle_from_sq<white_to_move, pieces::rook, moveflag::MOVEFLAG_long_castling>()) {
                move.flag = moveflag::MOVEFLAG_remove_long_castle;
            } else if (move.source == get_castle_from_sq<white_to_move, pieces::rook, moveflag::MOVEFLAG_short_castling>()) {
                move.flag = moveflag::MOVEFLAG_remove_short_castle;
            }
            break;
        }

        return do_move<white_to_move>(move);
    }
    template <bool white_to_move> restore_move_info constexpr inline do_move(Move &move) {
        assert(move.is_valid());
        Piece_t moved = get_piece_at(move.source).get_type();
        [[assume(moved >= 1 && moved <= 6)]];
        [[assume(move.flag >= 0 && move.flag <= 11)]];
        switch (moved) {
        case pieces::pawn:
            switch (move.flag) {
            case moveflag::MOVEFLAG_silent:
                return do_move<white_to_move, pieces::pawn, moveflag::MOVEFLAG_silent>(move);
            case moveflag::MOVEFLAG_pawn_double_push:
                return do_move<white_to_move, pieces::pawn, moveflag::MOVEFLAG_pawn_double_push, pieces::none>(move);
            case moveflag::MOVEFLAG_pawn_ep_capture:
                return do_move<white_to_move, pieces::pawn, moveflag::MOVEFLAG_pawn_ep_capture, pieces::none>(move);
            case moveflag::MOVEFLAG_promote_queen:
                return do_move<white_to_move, pieces::pawn, moveflag::MOVEFLAG_promote_queen>(move);
            case moveflag::MOVEFLAG_promote_bishop:
                return do_move<white_to_move, pieces::pawn, moveflag::MOVEFLAG_promote_bishop>(move);
            case moveflag::MOVEFLAG_promote_rook:
                return do_move<white_to_move, pieces::pawn, moveflag::MOVEFLAG_promote_rook>(move);
            case moveflag::MOVEFLAG_promote_knight:
                return do_move<white_to_move, pieces::pawn, moveflag::MOVEFLAG_promote_knight>(move);
            }
            break;
        case pieces::bishop:
            return do_move<white_to_move, pieces::bishop, moveflag::MOVEFLAG_silent>(move);
        case pieces::knight:
            return do_move<white_to_move, pieces::knight, moveflag::MOVEFLAG_silent>(move);
        case pieces::rook:
            switch (move.flag) {
            case moveflag::MOVEFLAG_silent:
                return do_move<white_to_move, pieces::rook, moveflag::MOVEFLAG_silent>(move);
            case moveflag::MOVEFLAG_remove_long_castle:
                return do_move<white_to_move, pieces::rook, moveflag::MOVEFLAG_remove_long_castle>(move);
            case moveflag::MOVEFLAG_remove_short_castle:
                return do_move<white_to_move, pieces::rook, moveflag::MOVEFLAG_remove_short_castle>(move);
            }
            break;
        case pieces::queen:
            return do_move<white_to_move, pieces::queen, moveflag::MOVEFLAG_silent>(move);
        case pieces::king:
            switch (move.flag) {
            case moveflag::MOVEFLAG_silent:
                return do_move<white_to_move, pieces::king, moveflag::MOVEFLAG_silent>(move);
            case moveflag::MOVEFLAG_remove_all_castle:
                return do_move<white_to_move, pieces::king, moveflag::MOVEFLAG_remove_all_castle>(move);
            case moveflag::MOVEFLAG_long_castling:
                return do_move<white_to_move, pieces::king, moveflag::MOVEFLAG_long_castling, pieces::none>(move);
            case moveflag::MOVEFLAG_short_castling:
                return do_move<white_to_move, pieces::king, moveflag::MOVEFLAG_short_castling, pieces::none>(move);
            }
            break;
        default:
            __builtin_unreachable();
        }
    }
    /**
     * @brief Intermediary do_move to select captured and enter templated function.
     *
     * @tparam white_to_move [TODO:description]
     * @tparam Piece_t [TODO:description]
     * @tparam Flag_t [TODO:description]
     * @param[[TODO:direction]] move [TODO:description]
     * @param[[TODO:direction]] captured [TODO:description]
     * @return [TODO:description]
     */
    template <bool white_to_move, Piece_t moved, Flag_t flag> restore_move_info constexpr inline do_move(Move &move) {
        Piece_t captured = get_piece_at(move.target).get_type();
        [[assume(captured >= 0 && captured <= 6 && captured != 1)]];
        switch (captured) {
        case pieces::none:
            return do_move<white_to_move, moved, flag, pieces::none>(move);
        case pieces::pawn:
            return do_move<white_to_move, moved, flag, pieces::pawn>(move);
        case pieces::bishop:
            return do_move<white_to_move, moved, flag, pieces::bishop>(move);
        case pieces::knight:
            return do_move<white_to_move, moved, flag, pieces::knight>(move);
        case pieces::rook:
            return do_move<white_to_move, moved, flag, pieces::rook>(move);
        case pieces::queen:
            return do_move<white_to_move, moved, flag, pieces::queen>(move);
        default:
            __builtin_unreachable();
        }
    }

    /**
     * @brief Removes castle flag if a captured piece was rook and that rook was in the corner
     * of the board
     *
     * @tparam white_to_move true if white MOVED. will remove black castle flags
     * @param[in] target target square
     */
    template <bool white_to_move> constexpr inline void remove_castle_flag_if_capture_rook(uint8_t target) {
        if constexpr (white_to_move) {
            if (target == get_castle_from_sq<false, pieces::rook, moveflag::MOVEFLAG_long_castling>()) {
                castleinfo &= ~castling::cast_black_queenside;
            } else if (target == get_castle_from_sq<false, pieces::rook, moveflag::MOVEFLAG_short_castling>()) {
                castleinfo &= ~castling::cast_black_kingside;
            }
        } else {
            if (target == get_castle_from_sq<true, pieces::rook, moveflag::MOVEFLAG_long_castling>()) {
                castleinfo &= ~castling::cast_white_queenside;
            } else if (target == get_castle_from_sq<true, pieces::rook, moveflag::MOVEFLAG_short_castling>()) {
                castleinfo &= ~castling::cast_white_kingside;
            }
        }
    }

    /**
     * @brief Final do move with flag (special move)
     *
     * @tparam white_to_move [TODO:description]
     * @tparam Piece_t [TODO:description]
     * @tparam Flag_t [TODO:description]
     * @tparam Piece_t [TODO:description]
     * @param[[TODO:direction]] move [TODO:description]
     * @return [TODO:description]
     */
    template <bool white_to_move, Piece_t moved, Flag_t flag, Piece_t captured> restore_move_info constexpr inline do_move(Move &move) {
        if constexpr (captured == pieces::none && moved != pieces::pawn)
            ply_moves += 1;
        else
            ply_moves = 0;
        if constexpr (!white_to_move)
            full_moves += 1;
        restore_move_info info = {castleinfo, captured, en_passant_square, ply_moves};

        uint8_t old_ep = en_passant_square;
        en_passant = false;
        en_passant_square = 0;
        assert(turn_color == white_to_move ? pieces::white : pieces::black);
        change_turn();  // Changes turn color from white <-> black.
        //
        assert(get_piece_at(move.source).get_color() == white_to_move ? pieces::white : pieces::black);

        if constexpr (captured != pieces::none) {
            remove_piece<!white_to_move, captured>(move.target);
            // Only need to check if a rook was captured SINCE if a piece wasnt captured and
            // we hit a rook square, the castleflag is already removed.
            if constexpr (captured == pieces::rook)
                remove_castle_flag_if_capture_rook<white_to_move>(move.target);
        }

        move_piece<white_to_move, moved>(move.source, move.target);

        if constexpr (flag == moveflag::MOVEFLAG_remove_all_castle || flag == moveflag::MOVEFLAG_short_castling || flag == moveflag::MOVEFLAG_long_castling) {
            if constexpr (white_to_move)
                castleinfo &= ~castling::cast_white_mask;
            else
                castleinfo &= ~castling::cast_black_mask;
        }
        if constexpr (flag == moveflag::MOVEFLAG_remove_long_castle) {
            if constexpr (white_to_move)
                castleinfo &= ~castling::cast_white_queenside;
            else
                castleinfo &= ~castling::cast_black_queenside;
        } else if constexpr (flag == moveflag::MOVEFLAG_remove_short_castle) {
            if constexpr (white_to_move)
                castleinfo &= ~castling::cast_white_kingside;
            else
                castleinfo &= ~castling::cast_black_kingside;
        }

        if constexpr (flag == moveflag::MOVEFLAG_long_castling || flag == moveflag::MOVEFLAG_short_castling) {
            assert(abs(static_cast<int>(move.source) - static_cast<int>(move.target)) == 2);
            move_piece<white_to_move, pieces::rook>(get_castle_from_sq<white_to_move, pieces::rook, flag>(),
                                                    get_castle_to_sq<white_to_move, pieces::rook, flag>());
        }

        if constexpr (flag == moveflag::MOVEFLAG_pawn_double_push) {
            en_passant = true;
            en_passant_square = (move.source + move.target) / 2;
        }

        if constexpr (flag == moveflag::MOVEFLAG_pawn_ep_capture) {
            if constexpr (white_to_move) {  // if white to move, the enemy pawn is on square
                                            // - 8
                remove_piece<!white_to_move, pieces::pawn>(old_ep - 8);
            } else {
                remove_piece<!white_to_move, pieces::pawn>(old_ep + 8);
            }
        }
        if constexpr (flag == moveflag::MOVEFLAG_promote_queen) {
            remove_piece<white_to_move, pieces::pawn>(move.target);
            add_piece<white_to_move, pieces::queen>(move.target);
        } else if constexpr (flag == moveflag::MOVEFLAG_promote_knight) {
            remove_piece<white_to_move, pieces::pawn>(move.target);
            add_piece<white_to_move, pieces::knight>(move.target);
        } else if constexpr (flag == moveflag::MOVEFLAG_promote_bishop) {
            remove_piece<white_to_move, pieces::pawn>(move.target);
            add_piece<white_to_move, pieces::bishop>(move.target);
        } else if constexpr (flag == moveflag::MOVEFLAG_promote_rook) {
            remove_piece<white_to_move, pieces::pawn>(move.target);
            add_piece<white_to_move, pieces::rook>(move.target);
        }
        return info;
    }

    template <bool white_to_move, Piece_t moved, Flag_t flag> constexpr inline uint8_t get_castle_from_sq() const {
        if constexpr (white_to_move) {
            if constexpr (moved == pieces::king) {
                return 4;
            } else {
                if constexpr (flag == moveflag::MOVEFLAG_long_castling)
                    return 0;
                else
                    return 7;
            }
        } else {
            if constexpr (moved == pieces::king) {
                return 60;
            } else {
                if constexpr (flag == moveflag::MOVEFLAG_long_castling)
                    return 56;
                else
                    return 63;
            }
        }
    }
    template <bool white_to_move, Piece_t moved, Flag_t flag> constexpr inline uint8_t get_castle_to_sq() {
        if constexpr (white_to_move) {
            if constexpr (moved == pieces::king) {
                if constexpr (flag == moveflag::MOVEFLAG_long_castling) {
                    return 2;
                } else {
                    return 6;
                }
            } else {
                if constexpr (flag == moveflag::MOVEFLAG_long_castling)
                    return 3;
                else
                    return 5;
            }
        } else {
            if constexpr (moved == pieces::king) {
                if constexpr (flag == moveflag::MOVEFLAG_long_castling) {
                    return 58;
                } else {
                    return 62;
                }
            } else {
                if constexpr (flag == moveflag::MOVEFLAG_long_castling)
                    return 59;
                else
                    return 61;
            }
        }
    }
    template <bool white_moved> void undo_move(const restore_move_info info, const Move move) {
        ply_moves = info.ply_moves;
        castleinfo = info.castleinfo;
        if (info.ep_square != 0) {
            en_passant_square = info.ep_square;
            en_passant = true;
        } else {
            en_passant = false;
            en_passant_square = 0;
        }
        if constexpr (!white_moved)
            full_moves -= 1;
        change_turn();
        Piece_t captured = info.captured;

        [[assume(move.flag >= 0 && move.flag <= 11)]];
        switch (move.flag) {
        case moveflag::MOVEFLAG_silent:
            undo_move<white_moved, moveflag::MOVEFLAG_silent>(move, captured);
            break;
        case moveflag::MOVEFLAG_pawn_ep_capture:
            undo_move<white_moved, pieces::pawn, moveflag::MOVEFLAG_pawn_ep_capture, pieces::none>(move);
            break;
        case moveflag::MOVEFLAG_long_castling:
            undo_move<white_moved, pieces::king, moveflag::MOVEFLAG_long_castling, pieces::none>(move);
            break;
        case moveflag::MOVEFLAG_short_castling:
            undo_move<white_moved, pieces::king, moveflag::MOVEFLAG_short_castling, pieces::none>(move);
            break;
        case moveflag::MOVEFLAG_promote_queen:
            undo_move<white_moved, pieces::pawn, moveflag::MOVEFLAG_promote_queen>(move, captured);
            break;
        case moveflag::MOVEFLAG_promote_knight:
            undo_move<white_moved, pieces::pawn, moveflag::MOVEFLAG_promote_knight>(move, captured);
            break;
        case moveflag::MOVEFLAG_promote_bishop:
            undo_move<white_moved, pieces::pawn, moveflag::MOVEFLAG_promote_bishop>(move, captured);
            break;
        case moveflag::MOVEFLAG_promote_rook:
            undo_move<white_moved, pieces::pawn, moveflag::MOVEFLAG_promote_rook>(move, captured);
            break;
        default:
            undo_move<white_moved, moveflag::MOVEFLAG_silent>(move, captured);
            break;
        }
    }

    inline constexpr void undo_move(restore_move_info info, const Move move) {
        if (turn_color == pieces::white)
            undo_move<false>(info, move);
        else
            undo_move<true>(info, move);
    }

    /**
     * @brief Calculates if king is in check.
     *
     * @param[in] turn_color Color of the king to be check if in check
     * @return True if checked, false if not in check
     */
    template <bool is_white> constexpr inline bool king_checked() const {
        BB king_bb = get_piece_bb<pieces::king, is_white>();
        uint8_t kingsq = BitBoard::lsb(king_bb);
        BB occ = occupancy();
        if ((movegen::knight_atk(kingsq) & get_piece_bb<pieces::knight, !is_white>()) > 0)
            return true;
        if ((movegen::bishop_atk(kingsq, occ) & (get_piece_bb<pieces::bishop, !is_white>() | get_piece_bb<pieces::queen, !is_white>())) > 0)
            return true;
        if ((movegen::rook_atk(kingsq, occ) & (get_piece_bb<pieces::rook, !is_white>() | get_piece_bb<pieces::queen, !is_white>())) > 0)
            return true;
        if ((movegen::pawn_atk_bb<is_white>(king_bb) & get_piece_bb<pieces::pawn, !is_white>()) > 0)
            return true;
        if ((movegen::king_atk(kingsq) & get_piece_bb<pieces::king, !is_white>()) > 0)
            return true;
        return false;
    }
    /**
     * @brief Calculates locations of enemy pieces checking king.
     * For use in setting flag of movegeneration.
     * Does not check for king attacks since only legal positions are allowed.
     *
     * @param[in] turn_color Color of the king to be check if in check
     * @return BB of the pieces checking the king.
     */
    template <bool is_white> constexpr inline BB king_checkers() const {
        BB king_bb = get_piece_bb<pieces::king, is_white>();
        uint8_t kingsq = BitBoard::lsb(king_bb);
        BB occ = occupancy();
        BB checkers = (movegen::knight_atk(kingsq) & get_piece_bb<pieces::knight, !is_white>());
        checkers |= movegen::bishop_atk(kingsq, occ) & (get_piece_bb<pieces::bishop, !is_white>() | get_piece_bb<pieces::queen, !is_white>());
        checkers |= movegen::rook_atk(kingsq, occ) & (get_piece_bb<pieces::rook, !is_white>() | get_piece_bb<pieces::queen, !is_white>());
        checkers |= movegen::pawn_atk_bb<is_white>(king_bb) & get_piece_bb<pieces::pawn, !is_white>();
        return checkers;
    }

    /**
     * @brief Get the all the possible legal moves and sets into provided array
     *
     * @tparam stype of move to generate.
     * @tparam is_white if is white or not.
     * @param moves array containing moves
     * @return * size_t: number of legal moves in array.
     */
    template <search_type stype, bool is_white> size_t get_moves(std::array<Move, max_legal_moves> &moves) {
        BB king_attackers = king_checkers<is_white>();
        uint8_t count = BitBoard::bitcount(king_attackers);
        if (count == 0) {
            return get_moves<stype, no_check, is_white>(moves, king_attackers);
        } else if (count == 1) {
            if (king_attackers &
                (get_piece_bb<pieces::bishop, !is_white>() | get_piece_bb<pieces::rook, !is_white>() | get_piece_bb<pieces::queen, !is_white>())) {
                return get_moves<stype, slider_check, is_white>(moves, king_attackers);
            } else {
                return get_moves<stype, single_check, is_white>(moves, king_attackers);
            }
        } else {
            return get_moves<stype, double_check, is_white>(moves, king_attackers);
        }
    }
    template <search_type stype, check_type ctype, bool is_white> size_t get_moves(std::array<Move, max_legal_moves> &moves, BB king_attackers) {
        uint8_t kingsq = BitBoard::lsb(get_piece_bb<pieces::king, is_white>());
        uint8_t color = is_white ? pieces::white : pieces::black;
        BB friendly_bb = occupancy<is_white>();
        BB enemy_bb = occupancy<!is_white>();
        BB to_squares = movegen::king_moves(kingsq, friendly_bb, friendly_bb | enemy_bb, get_atk_bb<!is_white, true>(), castleinfo,
                                            color);  // Already disqualifies squares that are attacked by the enemy so do not need to check for move legality.
        size_t num_moves = 0;
        add_moves<is_white, pieces::king>(moves, num_moves, to_squares, kingsq);
        // No need to test for move legality.
        if constexpr (ctype.two_checks) {
            return num_moves;
        } else {  // In case of check, valid moves are: Move king, block the checker if a ray piece, or capture the piece.
            size_t num_pseudolegal_moves = 0;
            BB occ = friendly_bb | enemy_bb;
            BB queen_bb = get_piece_bb<pieces::queen, is_white>();
            BB bishop_bb = get_piece_bb<pieces::bishop, is_white>();
            BB rook_bb = get_piece_bb<pieces::rook, is_white>();
            BB pawn_bb = get_piece_bb<pieces::pawn, is_white>();
            BB knight_bb = get_piece_bb<pieces::knight, is_white>();
            BB ep_bb = en_passant ? BitBoard::one_high(en_passant_square) : 0;

            // Compute pins
            BB rook_xraymask = magic::get_rook_xray_atk_bb(kingsq, occ);
            BB bishop_xraymask = magic::get_bishop_xray_atk_bb(kingsq, occ);
            BB enemy_rooks = get_piece_bb<pieces::rook, !is_white>() | get_piece_bb<pieces::queen, !is_white>();
            BB enemy_bishops = get_piece_bb<pieces::bishop, !is_white>() | get_piece_bb<pieces::queen, !is_white>();
            // If any enemy queens, bishops or rooks are present in these masks, pieces are pinned.
            BB pinning_rooks = enemy_rooks & rook_xraymask;
            BB pinrooktemp = pinning_rooks;
            BB pinning_bishops = enemy_bishops & bishop_xraymask;
            BB pinbishoptemp = pinning_bishops;
            // Generate a new mask only between king and enemy pinners, if any,
            BB rook_pinmask = 0;
            BB bishop_pinmask = 0;
            BitLoop(pinrooktemp) {
                uint8_t pinnerloc = BitBoard::lsb(pinrooktemp);
                rook_pinmask |= rect_lookup[kingsq][pinnerloc];
            }
            BitLoop(pinbishoptemp) {
                uint8_t pinnerloc = BitBoard::lsb(pinbishoptemp);
                bishop_pinmask |= rect_lookup[kingsq][pinnerloc];
            }
            pininfo pi = {kingsq, rook_pinmask, bishop_pinmask, pinning_rooks, pinning_bishops};

            // Compute checks

            // In the movegenerator, if the piece to be moved is on the rook or bishop pin mask,
            // then need to obtain the rectangular mask on which it is allowed to move.
            // This can be done by popping bits in the pinning_rooks BB / pinning_bishops BB, checking if this BB is between this piece and the king.
            // If it is, the piece is only allowed to move between king and the pinee.

            gen_add_all_moves<pieces::queen, stype, ctype, is_white>(moves, num_moves, queen_bb, friendly_bb, enemy_bb, pi, ep_bb, king_attackers);
            gen_add_all_moves<pieces::bishop, stype, ctype, is_white>(moves, num_moves, bishop_bb, friendly_bb, enemy_bb, pi, ep_bb, king_attackers);
            gen_add_all_moves<pieces::rook, stype, ctype, is_white>(moves, num_moves, rook_bb, friendly_bb, enemy_bb, pi, ep_bb, king_attackers);
            gen_add_all_moves<pieces::knight, stype, ctype, is_white>(moves, num_moves, knight_bb, friendly_bb, enemy_bb, pi, ep_bb, king_attackers);
            gen_add_all_moves<pieces::pawn, stype, ctype, is_white>(moves, num_moves, pawn_bb, friendly_bb, enemy_bb, pi, ep_bb, king_attackers);
            return num_moves;
        }
    }
    /**
     * @brief Changes whose turn it is: white <-> black. Only the turn_color parameter
     * is changed.
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
        en_passant_square = 0;
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

    inline Piece get_piece_at(uint8_t row, uint8_t col) const { return get_piece_at(NotationInterface::idx(row, col)); }

    inline Piece get_piece_at(uint8_t square) const { return game_board[square]; }

    bool is_square_empty(uint8_t square) const;

    uint8_t get_square_color(uint8_t square) const;

    void clear_board();

    /**
     * @brief Computes if a move acting on this board state would leave the king of a
     * certain color in check.
     *
     * @param[in] candidate candidate move to check if it leaves the king in check
     * @param[in] king_color color of king to check if is in check
     * @return boolean if king is in check or not
     */
    bool does_move_check(const Move candidate, const uint8_t king_color);
    /**
     * @brief Gets number of pieces for a player
     *
     * @tparam is_white white or black
     * @return number of pieces for selected player
     */
    template <bool is_white> inline constexpr uint8_t get_num_pieces() { return BitBoard::bitcount(occupancy<is_white>()); }
    inline constexpr uint8_t get_num_pieces() { return get_num_pieces<true>() + get_num_pieces<false>(); }
    void print_piece_loc() const;

    /**
     * @brief Gets the attack bitboard for a given color. These are squares that are
     * threatened by this player.
     *
     * @tparam exclude_kingocc - For if the opponent king should be xrayed through: useful if searching for king moves.
     * @param[in] color Color of player you want to get atk bitboard for
     * @return [ALl squares under attack by given player.]
     */
    template <bool for_white, bool exclude_kingocc> constexpr inline uint64_t get_atk_bb() const {
        BB friendly_pieces = occupancy<for_white>();
        BB enemy_pieces = occupancy<!for_white>();
        BB occ = friendly_pieces | enemy_pieces & ~(get_piece_bb<pieces::king, !for_white>());
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
    template <Piece_t pval, bool is_white> constexpr inline BB get_piece_bb() const {
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
    template <Piece_t pval> constexpr inline BB get_piece_bb(uint8_t col) const {
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
    template <Piece_t piece, bool is_white> constexpr inline uint8_t get_piece_cnt() const { return BitBoard::bitcount(get_piece_bb<piece, is_white>()); }

    constexpr inline BB occupancy() const { return occupancy<true>() | occupancy<false>(); }
    template <bool is_white> constexpr inline BB occupancy() const {
        if constexpr (is_white)
            return white_pieces;
        else
            return black_pieces;
    }
    /**
     * @brief Gets piece mobility for a given piece type
     *
     * @tparam Piece_t piece type
     * @tparam omit_pawn_controlled flag if we should omit squares controlled by enemy
     * pawns from the mobility count
     * @tparam is_white flag if piece to check is white or not
     * @return number of squares piece of this type and color can move to.
     */
    template <Piece_t pval, bool omit_pawn_controlled, bool is_white> constexpr int get_piece_mobility() const {  // TODO: Add EP squares, add castling.
        BB piece_bb = get_piece_bb<pval, is_white>();

        int mobility = 0;
        BB occ = occupancy();
        BB friendly = occupancy<is_white>();
        if constexpr (omit_pawn_controlled) {
            BB enemy_pawn_bb = get_piece_bb<pieces::pawn, !is_white>();
            BB enemy_pawn_atk = movegen::pawn_atk_bb<!is_white>(enemy_pawn_bb);
            BitLoop(piece_bb) {
                uint8_t sq = BitBoard::lsb(piece_bb);
                BB piece_atk_bb = movegen::get_atk_bb<pval, is_white>(sq, occ) & ~(friendly | enemy_pawn_atk);
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
    template <bool is_white> int get_pawn_promote_rank() const {
        if constexpr (is_white)
            return 7;
        else
            return 0;
    }
    /**
     * @brief From a bitboard of attacked squares, generate all moves and add to array
     *
     * @param[inout] moves array of moves to be added to
     * @param[inout] num_moves number of moves before routine on in, number of moves
     * after routine on out
     * @param[in] to_bb bitboard containing attacking squares. will be destroyed by
     * calling this routine
     * @param[in] from from square.
     */
    template <bool is_white, Piece_t type> void add_moves(std::array<Move, max_legal_moves> &moves, size_t &num_moves, uint64_t &to_bb, const uint8_t from) {
        constexpr uint8_t friendly_longsq = is_white ? 0 : 56;
        constexpr uint8_t friendly_shortsq = is_white ? 7 : 63;
        BitLoop(to_bb) {
            uint8_t lsb = BitBoard::lsb(to_bb);
            Flag_t flag = moveflag::MOVEFLAG_silent;
            if constexpr (type == pieces::pawn) {
                if (NotationInterface::row(lsb) == get_pawn_promote_rank<is_white>()) {
                    constexpr uint8_t col = is_white ? pieces::white : pieces::black;
                    for (uint8_t ptype : pieces::promote_types)
                        moves[num_moves++] = Move(from, lsb, Piece(ptype | col));

                    // flag set automatically in move constructor.
                    continue;
                } else {
                    if (lsb == en_passant_square) {
                        flag = moveflag::MOVEFLAG_pawn_ep_capture;
                        // Need to check for if move is legal or not.
                        Move testmove = Move(from, lsb, flag);
                        auto info = do_move<is_white, pieces::pawn, moveflag::MOVEFLAG_pawn_ep_capture, pieces::none>(testmove);
                        bool iskingcheck = king_checked<is_white>();
                        undo_move<is_white>(info, testmove);
                        if (iskingcheck)
                            continue;

                    } else if (abs(static_cast<int>(from) - static_cast<int>(lsb)) == 16) {
                        flag = moveflag::MOVEFLAG_pawn_double_push;
                    }
                }
            } else if constexpr (type == pieces::king) {
                if (lsb - from == 2) {
                    flag = moveflag::MOVEFLAG_short_castling;
                } else if (from - lsb == 2) {
                    flag = moveflag::MOVEFLAG_long_castling;
                } else if (from == get_castle_from_sq<is_white, pieces::king, moveflag::MOVEFLAG_long_castling>()) {
                    flag = moveflag::MOVEFLAG_remove_all_castle;
                }
            } else if constexpr (type == pieces::rook) {
                if (from == friendly_longsq)
                    flag = moveflag::MOVEFLAG_remove_long_castle;
                else if (from == friendly_shortsq)
                    flag = moveflag::MOVEFLAG_remove_short_castle;
            }
            moves[num_moves++] = Move(from, lsb, flag);
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
    template <Piece_t ptype, search_type stype, check_type ctype, bool is_white>
    void gen_add_all_moves(std::array<Move, max_legal_moves> &moves, size_t &num_moves, uint64_t &piece_bb, const uint64_t friendly_bb, const uint64_t enemy_bb,
                           const pininfo pi, const uint64_t ep_bb, const BB king_attacker) {
        BB checker_mask = ~0;
        if constexpr (ctype.slider_check) {
            checker_mask = rect_lookup[pi.kingloc][BitBoard::lsb(king_attacker)];  // Allowed to go in between, or to capture
        } else if constexpr (ctype.one_check) {
            checker_mask = king_attacker;  // allowed to capture.
            //
            // Special case: the king attacker might be a pawn which can be captured en_passant.
            if constexpr (ptype == pieces::pawn) {
                checker_mask |= ep_bb;
                // Sometimes will evaluate an en passant capture does not capture a checking pawn. Most of the time. However, I think that the cost of this
                // extra check is lower than the if-else statements of checking if it is a pawn that is checking the king.
                // The only case where an en-passant leads to a check is either the pawn checks, or its a discovered check.
                //
            }
        }
        BitLoop(piece_bb) {
            // Compute pins --------------------------
            BB pin_mask = ~0;                      // mask of allowed squares due to pins
            BB sqbb = BitBoard::lsb_bb(piece_bb);  // BB with only lsb.
            if (pi.rook_pinmask & sqbb) {
                // Need to find which piece is pinning us by looping through offending pieces
                BB temp_rooks = pi.rook_pinners;
                BitLoop(temp_rooks) {
                    uint8_t pinnersq = BitBoard::lsb(temp_rooks);
                    BB mask = rect_lookup[pi.kingloc][pinnersq];
                    if (mask & sqbb) {  //
                        pin_mask = mask;
                        break;
                    }
                }
            } else if (pi.bishop_pinmask & sqbb) {  // Cannot be both bishop & rook pinned.
                BB temp_bishops = pi.bishop_pinners;
                BitLoop(temp_bishops) {
                    uint8_t pinnersq = BitBoard::lsb(temp_bishops);
                    BB mask = rect_lookup[pi.kingloc][pinnersq];
                    if (mask & sqbb) {  //
                        pin_mask = mask;
                        break;
                    }
                }
            }  // End compute pins -----------------------

            uint8_t sq = BitBoard::lsb(piece_bb);
            uint64_t to_sqs = to_squares<ptype, stype, is_white>(sq, friendly_bb, enemy_bb, ep_bb, castleinfo);
            to_sqs &= pin_mask;
            if constexpr (ctype.one_check || ctype.slider_check) {
                to_sqs &= checker_mask;
            }
            add_moves<is_white, ptype>(moves, num_moves, to_sqs, sq);
        }
    }

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
    template <Piece_t ptype, search_type s_type, bool is_white> BB to_squares(uint8_t sq, BB friendly_bb, BB enemy_bb, BB ep_bb, uint8_t castleinfo) const {
        BB piece_bb = BitBoard::one_high(sq);
        constexpr uint8_t color = is_white ? pieces::white : pieces::black;
        BB to_squares;
        if constexpr (ptype == pieces::pawn) {
            to_squares = movegen::pawn_moves<is_white>(piece_bb, friendly_bb, enemy_bb, ep_bb);
        } else if constexpr (ptype == pieces::bishop) {
            to_squares = movegen::bishop_moves_sq(sq, friendly_bb, enemy_bb);
        } else if constexpr (ptype == pieces::knight) {
            to_squares = movegen::knight_moves(sq, friendly_bb);
        } else if constexpr (ptype == pieces::rook) {
            to_squares = movegen::rook_moves_sq(sq, friendly_bb, enemy_bb);
        } else if constexpr (ptype == pieces::queen) {
            to_squares = movegen::queen_moves_sq(sq, friendly_bb, enemy_bb);
        } else if constexpr (ptype == pieces::king) {
            to_squares = movegen::king_moves(sq, friendly_bb, friendly_bb | enemy_bb, get_atk_bb<!is_white, true>(), castleinfo, color);
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
    template <bool is_white, Piece_t piece> constexpr inline void bb_move(const uint8_t from, const uint8_t to) {
        bb_remove<is_white, piece>(from);
        bb_add<is_white, piece>(to);
    }
    /**
     * @brief Handles removing piece on bitboard.
     * before removing piece on board.
     *
     * @param[in] sq index of square to remove piece from
     * @param[in] piece to add
     */
    template <bool is_white, Piece_t piece> constexpr inline void bb_remove(const uint8_t sq) {
        BB bb = BitBoard::one_high(sq);
        if constexpr (is_white) {
            white_pieces &= ~bb;
            if constexpr (piece == pieces::pawn)
                white_pawns &= ~bb;
            else if constexpr (piece == pieces::bishop)
                white_bishops &= ~bb;
            else if constexpr (piece == pieces::rook)
                white_rooks &= ~bb;
            else if constexpr (piece == pieces::knight)
                white_knights &= ~bb;
            else if constexpr (piece == pieces::queen)
                white_queen &= ~bb;
            else if constexpr (piece == pieces::king)
                white_king &= ~bb;
        } else {
            black_pieces &= ~bb;
            if constexpr (piece == pieces::pawn)
                black_pawns &= ~bb;
            else if constexpr (piece == pieces::bishop)
                black_bishops &= ~bb;
            else if constexpr (piece == pieces::rook)
                black_rooks &= ~bb;
            else if constexpr (piece == pieces::knight)
                black_knights &= ~bb;
            else if constexpr (piece == pieces::queen)
                black_queen &= ~bb;
            else if constexpr (piece == pieces::king)
                black_king &= ~bb;
        }
    }
    /**
     * @brief Handles moving piece on bitboard.
     * board.
     *
     * @param[in] from index of from square
     * @param[in] piece to add
     */
    template <bool is_white, Piece_t piece> constexpr inline void bb_add(const uint8_t sq) {
        BB bb = BitBoard::one_high(sq);
        if constexpr (is_white) {
            white_pieces |= bb;
            if constexpr (piece == pieces::pawn)
                white_pawns |= bb;
            else if constexpr (piece == pieces::bishop)
                white_bishops |= bb;
            else if constexpr (piece == pieces::rook)
                white_rooks |= bb;
            else if constexpr (piece == pieces::knight)
                white_knights |= bb;
            else if constexpr (piece == pieces::queen)
                white_queen |= bb;
            else if constexpr (piece == pieces::king)
                white_king |= bb;
        } else {
            black_pieces |= bb;
            if constexpr (piece == pieces::pawn)
                black_pawns |= bb;
            else if constexpr (piece == pieces::bishop)
                black_bishops |= bb;
            else if constexpr (piece == pieces::rook)
                black_rooks |= bb;
            else if constexpr (piece == pieces::knight)
                black_knights |= bb;
            else if constexpr (piece == pieces::queen)
                black_queen |= bb;
            else if constexpr (piece == pieces::king)
                black_king |= bb;
        }
    }
    /**
     * @brief Loud undo moves. These are: Promotion, castling or ep capture.
     *
     * @tparam white_moved true if white moved
     * @tparam piece type of piece moved
     * @tparam flag flag to determine type of move
     * @tparam capture type of piece captured (during promotion)
     * @param[in] move move to restore
     */
    template <bool white_moved, Piece_t piece, Flag_t flag, Piece_t captured> constexpr void undo_move(const Move move) {
        if constexpr (flag == moveflag::MOVEFLAG_promote_queen) {
            add_piece<white_moved, pieces::pawn>(move.source);
            remove_piece<white_moved, pieces::queen>(move.target);
        } else if constexpr (flag == moveflag::MOVEFLAG_promote_knight) {
            add_piece<white_moved, pieces::pawn>(move.source);
            remove_piece<white_moved, pieces::knight>(move.target);
        } else if constexpr (flag == moveflag::MOVEFLAG_promote_rook) {
            add_piece<white_moved, pieces::pawn>(move.source);
            remove_piece<white_moved, pieces::rook>(move.target);
        } else if constexpr (flag == moveflag::MOVEFLAG_promote_bishop) {
            add_piece<white_moved, pieces::pawn>(move.source);
            remove_piece<white_moved, pieces::bishop>(move.target);
        } else {
            move_piece<white_moved, piece>(move.target, move.source);
        }
        if constexpr (flag == moveflag::MOVEFLAG_pawn_ep_capture) {
            if constexpr (white_moved)  // if white to move, the enemy pawn was on square - 8
                add_piece<!white_moved, pieces::pawn>(move.target - 8);
            else
                add_piece<!white_moved, pieces::pawn>(move.target + 8);

        } else if constexpr (captured != pieces::none) {
            // If capture: restore captured piece
            add_piece<!white_moved, captured>(move.target);
        }

        if constexpr (flag == moveflag::MOVEFLAG_long_castling) {
            move_piece<white_moved, pieces::rook>(get_castle_to_sq<white_moved, pieces::rook, moveflag::MOVEFLAG_long_castling>(),
                                                  get_castle_from_sq<white_moved, pieces::rook, moveflag::MOVEFLAG_long_castling>());
        }
        if constexpr (flag == moveflag::MOVEFLAG_short_castling) {
            move_piece<white_moved, pieces::rook>(get_castle_to_sq<white_moved, pieces::rook, moveflag::MOVEFLAG_short_castling>(),
                                                  get_castle_from_sq<white_moved, pieces::rook, moveflag::MOVEFLAG_short_castling>());
        }
    }

    template <bool white_moved, Flag_t flag> constexpr void undo_move(const Move move, const Piece_t captured) {
        Piece_t moved = get_piece_at(move.target).get_type();
        [[assume(moved >= 1 && moved <= 6)]];
        switch (moved) {
        case pieces::pawn:
            undo_move<white_moved, pieces::pawn, flag>(move, captured);
            break;
        case pieces::bishop:
            undo_move<white_moved, pieces::bishop, flag>(move, captured);
            break;
        case pieces::knight:
            undo_move<white_moved, pieces::knight, flag>(move, captured);
            break;
        case pieces::rook:
            undo_move<white_moved, pieces::rook, flag>(move, captured);
            break;
        case pieces::queen:
            undo_move<white_moved, pieces::queen, flag>(move, captured);
            break;
        case pieces::king:
            undo_move<white_moved, pieces::king, flag>(move, captured);
            break;
        default:
            __builtin_unreachable();
        }
    }
    template <bool white_moved, Piece_t piece, Flag_t flag> constexpr void undo_move(const Move move, const Piece_t captured) {
        [[assume(captured >= 0 && captured <= 6 && captured != 1)]];
        switch (captured) {
        case pieces::none:
            undo_move<white_moved, piece, flag, pieces::none>(move);
            break;
        case pieces::pawn:
            undo_move<white_moved, piece, flag, pieces::pawn>(move);
            break;
        case pieces::bishop:
            undo_move<white_moved, piece, flag, pieces::bishop>(move);
            break;
        case pieces::knight:
            undo_move<white_moved, piece, flag, pieces::knight>(move);
            break;
        case pieces::rook:
            undo_move<white_moved, piece, flag, pieces::rook>(move);
            break;
        case pieces::queen:
            undo_move<white_moved, piece, flag, pieces::queen>(move);
            break;
        default:
            __builtin_unreachable();
        }
    }
};
#endif
