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

    template <bool is_white, Piece_t type> void remove_piece(const uint8_t square) {
        bb_remove<is_white, type>(square);
        game_board[square] = none_piece;
        num_pieces--;
    }
    template <bool is_white, Piece_t type>
    void move_piece(const uint8_t source, const uint8_t target) {
        bb_move<is_white, type>(source, target);
        game_board[target] = game_board[source];
        game_board[source] = none_piece;
    }

    template <bool is_white, Piece_t type> void add_piece(const uint8_t square) {
        uint8_t color = is_white ? pieces::white : pieces::black;
        game_board[square] = Piece(color | type);
        bb_add<is_white, type>(square);
        num_pieces++;
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
        move.flag = moveflag::MOVEFLAG_silent;
        switch (moved) {
        case pieces::pawn:
            if (move.promotion.get_type() > 0) {
                switch (move.promotion.get_type()) {
                case (pieces::queen):
                    move.flag = moveflag::MOVEFLAG_promote_queen;
                    break;
                case (pieces::rook):
                    move.flag = moveflag::MOVEFLAG_promote_rook;
                    break;
                case (pieces::knight):
                    move.flag = moveflag::MOVEFLAG_promote_knight;
                    break;
                case (pieces::bishop):
                    move.flag = moveflag::MOVEFLAG_promote_bishop;
                    break;
                }
            } else {
                // Check for ep...
                if (move.target == en_passant_square)
                    move.flag = moveflag::MOVEFLAG_pawn_ep_capture;

                if constexpr (white_to_move) {
                    if (move.target - move.source == 16)
                        move.flag = moveflag::MOVEFLAG_pawn_double_push;
                } else {
                    if (move.source - move.target == 16)
                        move.flag = moveflag::MOVEFLAG_pawn_double_push;
                }
            }
            break;
        case pieces::king:
            if (static_cast<int>(move.target) - static_cast<int>(move.source) > 1) {
                if (move.target == get_castle_to_sq<white_to_move, pieces::king,
                                                    moveflag::MOVEFLAG_long_castling>())
                    move.flag = moveflag::MOVEFLAG_long_castling;
                else
                    move.flag = moveflag::MOVEFLAG_short_castling;
            } else if (move.source == get_castle_from_sq<white_to_move, pieces::king,
                                                         moveflag::MOVEFLAG_long_castling>())
                move.flag = moveflag::MOVEFLAG_remove_all_castle;
            break;
        case pieces::rook:
            if (move.source ==
                get_castle_from_sq<white_to_move, pieces::rook, moveflag::MOVEFLAG_long_castling>())
                move.flag = moveflag::MOVEFLAG_remove_long_castle;
            else if (move.source == get_castle_from_sq<white_to_move, pieces::rook,
                                                       moveflag::MOVEFLAG_short_castling>())
                move.flag = moveflag::MOVEFLAG_remove_short_castle;
            break;
        }

        return do_move<white_to_move>(move);
    }
    template <bool white_to_move> restore_move_info do_move(Move &move) {
        assert(move.is_valid());
        Piece_t moved = get_piece_at(move.source).get_type();
        Piece_t captured = get_piece_at(move.target).get_type();
        move.captured = Piece(captured | (white_to_move ? pieces::black : pieces::white));
        switch (moved) {
        case pieces::pawn:
            switch (move.flag) {
            case moveflag::MOVEFLAG_pawn_double_push:
                return do_move<white_to_move, pieces::pawn, moveflag::MOVEFLAG_pawn_double_push,
                               pieces::none>(move);
            case moveflag::MOVEFLAG_pawn_ep_capture:
                return do_move<white_to_move, pieces::pawn, moveflag::MOVEFLAG_pawn_ep_capture,
                               pieces::pawn>(move);
            case moveflag::MOVEFLAG_promote_queen:
                return do_move<white_to_move, pieces::pawn, moveflag::MOVEFLAG_promote_queen>(
                    move, captured);
            case moveflag::MOVEFLAG_promote_bishop:
                return do_move<white_to_move, pieces::pawn, moveflag::MOVEFLAG_promote_bishop>(
                    move, captured);
            case moveflag::MOVEFLAG_promote_rook:
                return do_move<white_to_move, pieces::pawn, moveflag::MOVEFLAG_promote_rook>(
                    move, captured);
            case moveflag::MOVEFLAG_promote_knight:
                return do_move<white_to_move, pieces::pawn, moveflag::MOVEFLAG_promote_knight>(
                    move, captured);
            }
            return do_move<white_to_move, pieces::pawn, moveflag::MOVEFLAG_silent>(move, captured);

        case pieces::rook:
            switch (move.flag) {
            case moveflag::MOVEFLAG_remove_long_castle:
                return do_move<white_to_move, pieces::rook, moveflag::MOVEFLAG_remove_long_castle>(
                    move, captured);
            case moveflag::MOVEFLAG_remove_short_castle:
                return do_move<white_to_move, pieces::rook, moveflag::MOVEFLAG_remove_short_castle>(
                    move, captured);
            }
            return do_move<white_to_move, pieces::rook, moveflag::MOVEFLAG_silent>(move, captured);
        case pieces::bishop:
            return do_move<white_to_move, pieces::bishop, moveflag::MOVEFLAG_silent>(move,
                                                                                     captured);
        case pieces::knight:
            return do_move<white_to_move, pieces::knight, moveflag::MOVEFLAG_silent>(move,
                                                                                     captured);
        case pieces::queen:
            return do_move<white_to_move, pieces::queen, moveflag::MOVEFLAG_silent>(move, captured);
        case pieces::king:
            switch (move.flag) {
            case moveflag::MOVEFLAG_remove_all_castle:
                return do_move<white_to_move, pieces::king, moveflag::MOVEFLAG_remove_all_castle>(
                    move, captured);
            case moveflag::MOVEFLAG_long_castling:
                return do_move<white_to_move, pieces::king, moveflag::MOVEFLAG_long_castling,
                               pieces::none>(move);
            case moveflag::MOVEFLAG_short_castling:
                return do_move<white_to_move, pieces::king, moveflag::MOVEFLAG_short_castling,
                               pieces::none>(move);
            }
            return do_move<white_to_move, pieces::king, moveflag::MOVEFLAG_silent>(move, captured);
        default:
            std::cerr << "Error: Piece moved must not be none: " << (int)moved << std::endl;
            std::cerr << "Move: " << move.toString() << std::endl;
            Display_board();
            throw std::runtime_error("moved none piece");
            exit(EXIT_FAILURE);
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
    template <bool white_to_move, Piece_t moved, Flag_t flag>
    restore_move_info do_move(Move &move, Piece_t captured) {
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
            throw std::runtime_error("Should not be allowed to capture a king.");
            break;
        }
    }

    /**
     * @brief Removes castle flag if a captured piece was rook and that rook was in the corner
     * of the board
     *
     * @tparam white_to_move true if white MOVED. will remove black castle flags
     * @param[in] target target square
     */
    template <bool white_to_move> void remove_castle_flag_if_capture_rook(uint8_t target) {
        if constexpr (white_to_move) {
            if (target ==
                get_castle_from_sq<false, pieces::rook, moveflag::MOVEFLAG_long_castling>()) {
                castleinfo &= ~castling::cast_black_queenside;
            } else if (target == get_castle_from_sq<false, pieces::rook,
                                                    moveflag::MOVEFLAG_short_castling>()) {
                castleinfo &= ~castling::cast_black_kingside;
            }
        } else {
            if (target ==
                get_castle_from_sq<true, pieces::rook, moveflag::MOVEFLAG_long_castling>()) {
                castleinfo &= ~castling::cast_white_queenside;
            } else if (target == get_castle_from_sq<true, pieces::rook,
                                                    moveflag::MOVEFLAG_short_castling>()) {
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
    template <bool white_to_move, Piece_t moved, Flag_t flag, Piece_t captured>
    restore_move_info do_move(Move &move) {
        if constexpr (captured == pieces::none && moved != pieces::pawn)
            ply_moves += 1;
        else
            ply_moves = 0;
        if constexpr (!white_to_move)
            full_moves += 1;
        restore_move_info info = {
            castleinfo, en_passant ? en_passant_square : err_val8, ply_moves,
            Piece(captured | (white_to_move ? pieces::black : pieces::white))};
        uint8_t old_ep = en_passant_square;
        en_passant = false;
        en_passant_square = err_val8;
        change_turn();  // Changes turn color from white <-> black.
        //
        if constexpr (flag == moveflag::MOVEFLAG_silent) {
            // No flag set so no special moves like castling or whatever.
            if constexpr (moved == pieces::king) {
                if constexpr (white_to_move) {
                    assert((castleinfo & castling::cast_white_mask) == 0);
                } else {
                    assert((castleinfo & castling::cast_black_mask) == 0);
                }
            }

            if constexpr (captured != pieces::none) {
                remove_piece<!white_to_move, captured>(move.target);
                // Only need to check if a rook was captured SINCE if a piece wasnt captured and
                // we hit a rook square, the castleflag is already removed.
                if constexpr (captured == pieces::rook)
                    remove_castle_flag_if_capture_rook<white_to_move>(move.target);
            }
            move_piece<white_to_move, moved>(move.source, move.target);
            return info;
        } else if constexpr (moved == pieces::king) {
            // One of the two castlings
            if constexpr (white_to_move)
                castleinfo &= ~castling::cast_white_mask;
            else
                castleinfo &= ~castling::cast_black_mask;

            move_piece<white_to_move, moved>(move.source, move.target);  // move the king...
            move_piece<white_to_move, pieces::rook>(
                get_castle_from_sq<white_to_move, pieces::rook, flag>(),
                get_castle_to_sq<white_to_move, pieces::rook, flag>());
            return info;

            // SPECIAL MOVES BELOW:
        } else if constexpr (moved == pieces::rook) {
            if constexpr (moveflag::MOVEFLAG_remove_long_castle) {
                if constexpr (white_to_move)
                    castleinfo &= ~castling::cast_white_queenside;
                else
                    castleinfo &= ~castling::cast_black_queenside;
            } else {
                if constexpr (white_to_move)
                    castleinfo &= ~castling::cast_white_kingside;
                else
                    castleinfo &= ~castling::cast_black_kingside;
            }
            if constexpr (captured != pieces::none) {
                remove_piece<!white_to_move, captured>(move.target);
                if constexpr (captured == pieces::rook) {
                    remove_castle_flag_if_capture_rook<white_to_move>(move.target);
                }
            }
            move_piece<white_to_move, moved>(move.source, move.target);
        } else if constexpr (moved == pieces::pawn) {
            // Double pawn push.
            if constexpr (flag == moveflag::MOVEFLAG_pawn_double_push) {
                move_piece<white_to_move, moved>(move.source, move.target);
                en_passant = true;
                en_passant_square = (move.source + move.target) / 2;
            } else if constexpr (flag == moveflag::MOVEFLAG_pawn_ep_capture) {
                move_piece<white_to_move, moved>(move.source, move.target);
                if constexpr (white_to_move) {  // if white to move, the enemy pawn is on square
                                                // - 8
                    remove_piece<!white_to_move, pieces::pawn>(old_ep - 8);
                } else {
                    remove_piece<!white_to_move, pieces::pawn>(old_ep + 8);
                }
            } else {
                if constexpr (captured != pieces::none) {
                    remove_piece<!white_to_move, captured>(move.target);
                    if constexpr (captured == pieces::rook) {
                        remove_castle_flag_if_capture_rook<white_to_move>(move.target);
                    }
                }

                remove_piece<white_to_move, pieces::pawn>(move.source);
                if constexpr (flag == moveflag::MOVEFLAG_promote_queen)
                    add_piece<white_to_move, pieces::queen>(move.target);
                else if constexpr (flag == moveflag::MOVEFLAG_promote_knight)
                    add_piece<white_to_move, pieces::knight>(move.target);
                else if constexpr (flag == moveflag::MOVEFLAG_promote_bishop)
                    add_piece<white_to_move, pieces::bishop>(move.target);
                else if constexpr (flag == moveflag::MOVEFLAG_promote_rook)
                    add_piece<white_to_move, pieces::rook>(move.target);
            }
        }
        return info;
    }

    template <bool white_to_move, Piece_t moved, Flag_t flag>
    constexpr uint8_t get_castle_from_sq() {
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
    template <bool white_to_move, Piece_t moved, Flag_t flag> constexpr uint8_t get_castle_to_sq() {
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

    template <bool white_moved> void undo_move(restore_move_info info, const Move move) {
        ply_moves = info.ply_moves;
        castleinfo = info.castleinfo;
        if (info.ep_square != err_val8) {
            en_passant_square = info.ep_square;
            en_passant = true;
        }
        change_turn();
        if (move.flag == moveflag::MOVEFLAG_silent) {
            // Quiet moves. These are just captures or moves with no special effects.
            Piece_t captured = info.captured.get_type();
            switch (get_piece_at(move.target).get_type()) {
            case (pieces::pawn):
                undo_move<white_moved, pieces::pawn, moveflag::MOVEFLAG_silent>(move, captured);
                break;
            case (pieces::bishop):
                undo_move<white_moved, pieces::bishop, moveflag::MOVEFLAG_silent>(move, captured);
                break;
            case (pieces::knight):
                undo_move<white_moved, pieces::knight, moveflag::MOVEFLAG_silent>(move, captured);
                break;
            case (pieces::queen):
                undo_move<white_moved, pieces::queen, moveflag::MOVEFLAG_silent>(move, captured);
                break;
            case (pieces::rook):
                undo_move<white_moved, pieces::rook, moveflag::MOVEFLAG_silent>(move, captured);
                break;
            case (pieces::king):
                undo_move<white_moved, pieces::king, moveflag::MOVEFLAG_silent>(move, captured);
                break;
            default:
                Display_board();
                std::cerr << "Move to unmake crashed: " << move.toString() << std::endl;
                throw std::runtime_error("Undefined piece");
                break;
            }
        } else {
            // Can be rook, king or pawn.
            Piece_t dest = get_piece_at(move.target).get_type();
            if (dest == pieces::pawn) {
                // EP capture, double pawn push.
                if (move.flag == moveflag::MOVEFLAG_pawn_ep_capture) {
                    undo_move<white_moved, pieces::pawn, moveflag::MOVEFLAG_pawn_ep_capture,
                              pieces::pawn>(move);
                } else if (move.flag == moveflag::MOVEFLAG_pawn_double_push) {
                    undo_move<white_moved, pieces::pawn, moveflag::MOVEFLAG_silent, pieces::none>(
                        move);
                } else {
                    throw std::runtime_error("Moveflag did not have one of the accepted values");
                }
            } else if (dest == pieces::king) {
                if (move.flag == moveflag::MOVEFLAG_short_castling) {
                    undo_move<white_moved, pieces::king, moveflag::MOVEFLAG_short_castling,
                              pieces::none>(move);
                } else if (move.flag == moveflag::MOVEFLAG_long_castling) {
                    undo_move<white_moved, pieces::king, moveflag::MOVEFLAG_short_castling,
                              pieces::none>(move);
                } else if (move.flag == moveflag::MOVEFLAG_remove_all_castle) {
                    Piece_t captured = info.captured.get_type();
                    undo_move<white_moved, pieces::king, moveflag::MOVEFLAG_silent>(
                        move,
                        captured);  // INFO:This is fine since the restoring castle is restored
                                    // by restoreinfo. flag does not need to be set to special.
                } else {
                    throw std::runtime_error("Moveflag did not have one of the accepted values");
                }

                // castling / just move and set kingsq low.
            } else {  // here we can have rook and promotion of pawn.
                // Would like to avoid as many if statements as possible.
                // Either: Check for if rook AND not promotion rook flag: do rook moves. Then we
                // know its a pawn promotion.
                Piece_t captured = info.captured.get_type();
                if (dest == pieces::rook && (move.flag != moveflag::MOVEFLAG_promote_rook)) {
                    undo_move<white_moved, pieces::rook, moveflag::MOVEFLAG_silent>(
                        move, captured);  // INFO: This is fine. The rook moved from its castle
                                          // square but castleinfo already restored.
                } else {                  // promotion.
                    if (move.flag == moveflag::MOVEFLAG_promote_queen) {
                        undo_move<white_moved, pieces::pawn, moveflag::MOVEFLAG_promote_queen>(
                            move, captured);
                    } else if (move.flag == moveflag::MOVEFLAG_promote_knight) {
                        undo_move<white_moved, pieces::pawn, moveflag::MOVEFLAG_promote_knight>(
                            move, captured);
                    } else if (move.flag == moveflag::MOVEFLAG_promote_bishop) {
                        undo_move<white_moved, pieces::pawn, moveflag::MOVEFLAG_promote_bishop>(
                            move, captured);
                    } else if (move.flag == moveflag::MOVEFLAG_promote_rook) {
                        undo_move<white_moved, pieces::pawn, moveflag::MOVEFLAG_promote_rook>(
                            move, captured);
                    } else {
                        throw std::runtime_error(
                            "Moveflag entered promoton branch but no promotion was set.");
                    }
                }
            }
        }
    }

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
                     // PERF: Implement method in Board that only does/undoes moves in the
                     // bitboards for performance.
            restore_move_info info = do_move<is_white>(pseudolegal_moves[m]);
            if (!king_checked<is_white>()) {
                // PERF: Check how expensive this king_checked thing is. Is it worth the move
                // ordering benefit?
                // bool opponent_checked = board.king_checked(opposite_color);
                // pseudolegal_moves[m].check = opponent_checked;
                moves[num_moves++] = pseudolegal_moves[m];
            }
            undo_move<is_white>(info, pseudolegal_moves[m]);
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
     * @brief Gets the attack bitboard for a given color. These are squares that are
     * threatened by this player.
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
     * @tparam omit_pawn_controlled flag if we should omit squares controlled by enemy
     * pawns from the mobility count
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
    template <bool is_white, Piece_t type>
    void add_moves(std::array<Move, max_legal_moves> &moves, size_t &num_moves, uint64_t &to_bb,
                   const uint8_t from) const {
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

    std::array<Piece, 64> game_board;
    // Color                 W          B
    // Bitboards: Pieces: [9-14]   [17-22].
    //            Attack: 15         23
    //            All p : 8          16

    std::array<uint64_t, 32> bit_boards{};
    BB white_rooks, white_pawns, white_knights, white_bishops, white_queen, white_king,
        white_pieces;
    BB black_rooks, black_pawns, black_knights, black_bishops, black_queen, black_king,
        black_pieces;

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
    template <bool is_white, Piece_t piece> void bb_move(const uint8_t from, const uint8_t to) {
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
    template <bool is_white, Piece_t piece> void bb_remove(const uint8_t sq) {
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
    template <bool is_white, Piece_t piece> void bb_add(const uint8_t sq) {
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
    template <bool white_moved, Piece_t piece, Flag_t flag, Piece_t captured>
    void undo_move(const Move move) {
        if constexpr (flag == moveflag::MOVEFLAG_silent) {
            // Just captures and normal moves.
            move_piece<white_moved, piece>(move.target, move.source);
            if constexpr (captured != pieces::none) {
                // If capture: restore captured piece
                add_piece<!white_moved, captured>(move.target);
            }

        } else if constexpr (flag == moveflag::MOVEFLAG_pawn_ep_capture) {
            move_piece<white_moved, piece>(move.target, move.source);
            if constexpr (white_moved) {  // if white to move, the enemy pawn was on square - 8
                add_piece<white_moved, pieces::pawn>(move.target - 8);
            } else {
                add_piece<white_moved, pieces::pawn>(move.target + 8);
            }
        } else if constexpr (flag == moveflag::MOVEFLAG_long_castling) {
            move_piece<white_moved, pieces::king>(move.target, move.source);
            // King moved
            move_piece<white_moved, pieces::rook>(
                get_castle_to_sq<white_moved, pieces::rook, moveflag::MOVEFLAG_long_castling>(),
                get_castle_from_sq<white_moved, pieces::rook, moveflag::MOVEFLAG_long_castling>());
            // rook moved.

        } else if constexpr (flag == moveflag::MOVEFLAG_short_castling) {
            move_piece<white_moved, pieces::king>(move.target, move.source);
            // King moved
            move_piece<white_moved, pieces::rook>(
                get_castle_to_sq<white_moved, pieces::rook, moveflag::MOVEFLAG_short_castling>(),
                get_castle_from_sq<white_moved, pieces::rook, moveflag::MOVEFLAG_short_castling>());
        } else {  // Should only be promotions here. these could have captured.
            static_assert(piece == pieces::pawn,
                          "Moveflag entered promotion branch without moving a pawn");

            add_piece<white_moved, pieces::pawn>(move.source);
            if constexpr (flag == moveflag::MOVEFLAG_promote_queen) {
                remove_piece<white_moved, pieces::queen>(move.target);
            } else if constexpr (flag == moveflag::MOVEFLAG_promote_knight) {
                remove_piece<white_moved, pieces::knight>(move.target);
            } else if constexpr (flag == moveflag::MOVEFLAG_promote_rook) {
                remove_piece<white_moved, pieces::rook>(move.target);
            } else if constexpr (flag == moveflag::MOVEFLAG_promote_bishop) {
                remove_piece<white_moved, pieces::bishop>(move.target);
            } else {
                throw std::runtime_error(
                    "Moveflag entered promoton branch but no promotion was set.");
            }
            if constexpr (captured != pieces::none)
                add_piece<!white_moved, captured>(move.target);
        }
    }
    template <bool white_moved, Piece_t piece, Flag_t flag>
    void undo_move(const Move move, Piece_t captured) {
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
            throw std::runtime_error("Should not be allowed to capture a king.");
            break;
        }
    }
};
#endif
