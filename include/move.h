// Copyright 2025 Filip Agert
#ifndef MOVE_H
#define MOVE_H
#include <constants.h>
#include <notation_interface.h>
#include <piece.h>

#include <cstdint>
#include <string>
using Flag_t = uint8_t;
namespace moveflag {
const Flag_t MOVEFLAG_silent = 0;
const Flag_t MOVEFLAG_remove_short_castle = 1;
const Flag_t MOVEFLAG_remove_long_castle = 2;
const Flag_t MOVEFLAG_remove_all_castle = 3;
const Flag_t MOVEFLAG_pawn_double_push = 4;
const Flag_t MOVEFLAG_pawn_ep_capture = 5;
const Flag_t MOVEFLAG_promote_queen = 6;
const Flag_t MOVEFLAG_promote_bishop = 7;
const Flag_t MOVEFLAG_promote_rook = 8;
const Flag_t MOVEFLAG_promote_knight = 9;
const Flag_t MOVEFLAG_short_castling = 10;
const Flag_t MOVEFLAG_long_castling = 11;
template <Flag_t flag> constexpr bool is_promotion() {
    if constexpr ((flag == MOVEFLAG_promote_queen) || (flag == MOVEFLAG_promote_bishop) ||
                  (flag == MOVEFLAG_promote_knight) || (flag == MOVEFLAG_promote_rook))
        return true;
    else
        return false;
}
}  // namespace moveflag
struct Move {
    Piece captured = none_piece;   // captured.
    Piece promotion = none_piece;  // Piece to be promoted into
    uint8_t source = err_val8, target = err_val8;
    Flag_t flag = 0;
    uint8_t en_passant_square = err_val8;
    uint8_t castling_rights;
    uint8_t ply;
    bool check = false;  // Flag if move leads to check or not. For use in move order selection.

    explicit Move(std::string move_str) {
        if (move_str.length() < 4 || move_str.length() > 5)
            throw std::invalid_argument("Move string invalid: " + move_str);
        int source = NotationInterface::idx_from_string(move_str.substr(0, 2));
        int target = NotationInterface::idx_from_string(move_str.substr(2, 2));
        this->source = source;
        this->target = target;
        if (move_str.length() == 5) {
            uint8_t color = NotationInterface::row(target) == 0 ? pieces::black : pieces::white;
            Piece p = Piece(Piece::piece_type_from_char(move_str[4]) | color);
            this->promotion = p;
        }
    }
    constexpr Move(uint8_t from, uint8_t to) {
        this->source = from;
        this->target = to;
        this->promotion = Piece();
        this->castling_rights = 0;
        this->flag = 0;
        this->ply = 0;
        this->check = false;
    }
    constexpr Move(uint8_t from, uint8_t to, Flag_t flagval) : Move(from, to) {
        this->flag = flagval;
    }
    constexpr Move(uint8_t from, uint8_t to, Piece promo) : Move(from, to) {
        promotion = promo;
        switch (promotion.get_type()) {
        case (pieces::queen):
            flag = moveflag::MOVEFLAG_promote_queen;
            break;
        case (pieces::knight):
            flag = moveflag::MOVEFLAG_promote_knight;
            break;
        case (pieces::rook):
            flag = moveflag::MOVEFLAG_promote_rook;
            break;
        case (pieces::bishop):
            flag = moveflag::MOVEFLAG_promote_bishop;
            break;
        }
    }
    std::string toString() const {
        if (source == err_val8)
            return "0000";

        std::string out =
            NotationInterface::string_from_idx(source) + NotationInterface::string_from_idx(target);
        if (!(promotion == none_piece))
            out += promotion.get_char_lc();
        return out;
    }
    constexpr inline bool is_valid() {
        return (source != target) && (source != err_val8) && (target != err_val8);
    }
    constexpr Move() {}
};

#endif
