// Copyright 2025 Filip Agert
#ifndef MOVE_H
#define MOVE_H
#include <constants.h>
#include <notation_interface.h>
#include <piece.h>

#include <cstdint>
#include <string>
struct Move {
    Piece captured = none_piece;  // captured.
    Piece promotion = none_piece;       // Piece to be promoted into
    uint8_t source = err_val8, target = err_val8;
    uint8_t en_passant_square = err_val8;
    uint8_t castling_rights;
    uint8_t ply;
    bool check = false;  // Flag if move leads to check or not. For use in move order selection.

    explicit Move(std::string move_str) {
        if (move_str.length() < 4 || move_str.length() > 5)
            throw new std::invalid_argument("Move string invalid: " + move_str);
        this->source = NotationInterface::idx_from_string(move_str.substr(0, 2));
        this->target = NotationInterface::idx_from_string(move_str.substr(2, 2));
        if (move_str.length() == 5) {
            uint8_t color = NotationInterface::row(target) == 0 ? pieces::black : pieces::white;
            Piece p = Piece(Piece::piece_type_from_char(move_str[4]) | color);
            this->promotion = p;
        }
    }
    constexpr Move(uint8_t from, uint8_t to) {
        source = from;
        target = to;
    }
    constexpr Move(uint8_t from, uint8_t to, Piece promo) {
        source = from;
        target = to;
        promotion = promo;
    }
    std::string toString() const {
        if (source == err_val8)
            return "0000";

        std::string out = NotationInterface::string_from_idx(source) +
                          NotationInterface::string_from_idx(target);
        if (!(promotion == none_piece))
            out += promotion.get_char_lc();
        return out;
    }
    Move() {}
};

#endif
