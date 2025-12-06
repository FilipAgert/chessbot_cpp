// Copyright 2025 Filip Agert
#include "piece.h"
#include <cstdint>
#include <string>
using namespace pieces;
const std::string Piece::white_pieces = "KQRNBP";
const std::string Piece::black_pieces = "kqrnbp";
const std::string Piece::all_piece_symbols = " kqrnbp wKQRNBP bkqrnbp";
char Piece::get_char() const { return all_piece_symbols[value]; }
char Piece::get_char_lc() const { return all_piece_symbols[this->get_type()]; }

Piece::Piece(char c) {
    uint8_t color = piece_col_from_char(c);
    uint8_t type = piece_type_from_char(c);
    this->value = (color | type);
}

uint8_t Piece::piece_col_from_char(char c) {
    if (Piece::piece_type_from_char(c) == none) {  // First check if the character is a valid Piece
        return none;
    }

    if (c >= 'A' && c <= 'Z') {
        return white;
    } else if (c >= 'a' && c <= 'z') {
        return black;
    }
    return none;  // Return none if the character is not a valid Piece
}

uint8_t Piece::piece_type_from_char(char c) {
    if (c == 'K' || c == 'k') {
        return king;
    } else if (c == 'Q' || c == 'q') {
        return queen;
    } else if (c == 'R' || c == 'r') {
        return rook;
    } else if (c == 'N' || c == 'n') {
        return knight;
    } else if (c == 'B' || c == 'b') {
        return bishop;
    } else if (c == 'P' || c == 'p') {
        return pawn;
    }
    return none;  // Return none if the character is not a valid Piece
}
