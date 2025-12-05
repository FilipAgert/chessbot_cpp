// Copyright 2025 Filip Agert
#include <notation_interface.h>
#include <string>

using namespace pieces;
std::string NotationInterface::castling_rights(const uint8_t castle) {
    std::string builder = "";
    if (castle & castling::cast_white_kingside)
        builder.push_back(Piece(king | white).get_char());
    if (castle & castling::cast_white_queenside)
        builder.push_back(Piece(queen | white).get_char());
    if (castle & castling::cast_black_kingside)
        builder.push_back(Piece(king | black).get_char());
    if (castle & castling::cast_black_queenside)
        builder.push_back(Piece(queen | black).get_char());
    if (builder.length() == 0)
        builder = "-";
    return builder;
}

std::string NotationInterface::string_from_idx(const uint8_t idx) {
    uint8_t row, col;
    row_col(row, col, idx);
    char r = 'a' + col;
    char c = '1' + row;
    std::string s = std::string() + r + c;
    return s;
}

void NotationInterface::row_col(uint8_t &row, uint8_t &col, const uint8_t idx) {
    col = idx % 8;
    row = idx / 8;
}
