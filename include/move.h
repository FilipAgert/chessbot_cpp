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
template <Flag_t flag> static constexpr bool is_promotion() {
    if constexpr ((flag == MOVEFLAG_promote_queen) || (flag == MOVEFLAG_promote_bishop) || (flag == MOVEFLAG_promote_knight) || (flag == MOVEFLAG_promote_rook))
        return true;
    else
        return false;
}
inline constexpr bool is_promotion(Flag_t flag) {
    return ((flag == MOVEFLAG_promote_queen) || (flag == MOVEFLAG_promote_bishop) || (flag == MOVEFLAG_promote_knight) || (flag == MOVEFLAG_promote_rook));
}
}  // namespace moveflag
struct Move {
    Flag_t flag : 4 = 0;
    uint8_t source : 6 = 0;
    uint8_t target : 6 = 0;

    inline constexpr bool is_promotion() { return moveflag::is_promotion(flag); }
    inline constexpr Piece_t get_promotion() {
        if (flag == moveflag::MOVEFLAG_promote_queen)
            return pieces::queen;
        else if (flag == moveflag::MOVEFLAG_promote_knight)
            return pieces::knight;
        else if (flag == moveflag::MOVEFLAG_promote_rook)
            return pieces::rook;
        else if (flag == moveflag::MOVEFLAG_promote_bishop)
            return pieces::bishop;
        return pieces::none;
    }

    explicit Move(std::string move_str) {
        if (move_str.length() < 4 || move_str.length() > 5)
            throw std::invalid_argument("Move string invalid: " + move_str);
        int source = NotationInterface::idx_from_string(move_str.substr(0, 2));
        int target = NotationInterface::idx_from_string(move_str.substr(2, 2));
        this->source = source;
        this->target = target;
        if (move_str.length() == 5) {
            switch (Piece::piece_type_from_char(move_str[4])) {
            case pieces::queen:
                this->flag = moveflag::MOVEFLAG_promote_queen;
                break;
            case pieces::knight:
                this->flag = moveflag::MOVEFLAG_promote_knight;
                break;
            case pieces::rook:
                this->flag = moveflag::MOVEFLAG_promote_rook;
                break;
            case pieces::bishop:
                this->flag = moveflag::MOVEFLAG_promote_bishop;
                break;
            }
        }
    }
    constexpr Move(uint8_t from, uint8_t to) {
        this->source = from;
        this->target = to;
        this->flag = 0;
    }
    constexpr Move(uint8_t from, uint8_t to, Flag_t flagval) : Move(from, to) { this->flag = flagval; }
    constexpr Move(uint8_t from, uint8_t to, Piece promo) : Move(from, to) {
        switch (promo.get_type()) {
        case (pieces::queen):
            this->flag = moveflag::MOVEFLAG_promote_queen;
            break;
        case (pieces::knight):
            this->flag = moveflag::MOVEFLAG_promote_knight;
            break;
        case (pieces::rook):
            this->flag = moveflag::MOVEFLAG_promote_rook;
            break;
        case (pieces::bishop):
            this->flag = moveflag::MOVEFLAG_promote_bishop;
            break;
        }
    }
    std::string toString() const {
        if (source == target)
            return "0000";

        std::string out = NotationInterface::string_from_idx(source) + NotationInterface::string_from_idx(target);
        if (flag == moveflag::MOVEFLAG_promote_rook)
            out += 'r';
        else if (flag == moveflag::MOVEFLAG_promote_bishop)
            out += 'b';
        else if (flag == moveflag::MOVEFLAG_promote_queen)
            out += 'q';
        else if (flag == moveflag::MOVEFLAG_promote_knight)
            out += 'n';
        return out;
    }
    constexpr inline bool is_valid() { return (source != target); }
    constexpr inline bool operator==(const Move &other) const { return (source == other.source) && (target == other.target) && (flag == other.flag); }
    constexpr Move() {}
};

#endif
