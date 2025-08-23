// board_test.cpp
#include "constants.h"
#include <board.h>
#include <gtest/gtest.h>
#include <string>

// Test that board() initializes all squares to empty pieces
TEST(BoardTest, Initialization) {
    board b = board();
    for (int i = 0; i < 64; ++i) {
        ASSERT_EQ(b.get_piece_at(i).get_type(), piece::none);
    }
}

TEST(BoardTest, AddPiece) {
    board b = board();
    piece p = piece(piece::white | piece::rook);
    b.add_piece(8, p);
    ASSERT_EQ(b.get_piece_at(8), p);
}

TEST(BoardTest, MovePiece) {
    board b = board();
    piece p1 = piece(piece::white | piece::rook);
    piece p2 = piece(piece::black | piece::knight);
    b.add_piece(8, p1);
    b.add_piece(16, p2);
    b.move_piece(8, 16);
    ASSERT_EQ(b.get_piece_at(16), p1);
    ASSERT_EQ(b.get_piece_at(8).get_type(), piece::none);
}

TEST(BoardTest, RemovePiece) {
    board b = board();
    piece p = piece(piece::white | piece::rook);
    b.add_piece(8, p);
    b.remove_piece(8);
    ASSERT_EQ(b.get_piece_at(8).get_type(), piece::none);

    b.remove_piece(9);
    ASSERT_EQ(b.get_piece_at(9), piece::none);
}

TEST(BoardTest, idx){
    uint8_t row = 0;
    uint8_t col = 0; //A1
    uint8_t idx_val = board::idx(row, col);
    ASSERT_EQ(idx_val, 0);
    row = 7; //H8
    col = 7;
    idx_val = board::idx(row, col);
    ASSERT_EQ(idx_val, 63);
    row = 1; // A2
    col = 0;
    idx_val = board::idx(row, col);
    ASSERT_EQ(idx_val, 8);
    row = 0; // B1
    col = 1;
    idx_val = board::idx(row, col);
    ASSERT_EQ(idx_val, 1);

    col = 7; //H1
    row = 0;
    idx_val = board::idx(row, col);
    ASSERT_EQ(idx_val, 7);
}

TEST(BoardTest, idx_from_string){
    std::string idx = "A1";
    uint8_t idx_val = board::idx_from_string(idx);
    ASSERT_EQ(idx_val, 0);

    idx = "H8";
    idx_val = board::idx_from_string(idx);
    ASSERT_EQ(idx_val, 63);

    idx = "A8";
    idx_val = board::idx_from_string(idx);
    ASSERT_EQ(idx_val, 56);


    idx = "H1";
    idx_val = board::idx_from_string(idx);
    ASSERT_EQ(idx_val, 7);
    idx = "A9";
    idx_val = board::idx_from_string(idx);
    ASSERT_EQ(idx_val, err_val8);
    idx = "I1";
    idx_val = board::idx_from_string(idx);
    ASSERT_EQ(idx_val, err_val8);
    idx = "A0";
    idx_val = board::idx_from_string(idx);
    ASSERT_EQ(idx_val, err_val8);
    idx = "A";
    idx_val = board::idx_from_string(idx);
    ASSERT_EQ(idx_val, err_val8);
    idx = "1";
    idx_val = board::idx_from_string(idx);
    ASSERT_EQ(idx_val, err_val8);
    idx = "A1A";
    idx_val = board::idx_from_string(idx);
    ASSERT_EQ(idx_val, err_val8);
}

TEST(BoardTest, is_square_empty) {
    board b = board();
    piece p = piece(piece::white | piece::rook);
    b.add_piece(8, p);
    ASSERT_FALSE(b.is_square_empty(8));
    ASSERT_TRUE(b.is_square_empty(9));
}

TEST(BoardTest, get_square_color) {
    board b = board();
    piece p1 = piece(piece::white | piece::rook);
    piece p2 = piece(piece::black | piece::knight);
    b.add_piece(8, p1);
    b.add_piece(16, p2);
    ASSERT_EQ(b.get_square_color(8), piece::white);
    ASSERT_EQ(b.get_square_color(16), piece::black);
}

TEST(BoardTest, clear_board) {
    board b = board();
    piece p = piece(piece::white | piece::rook);
    b.add_piece(8, p);
    b.clear_board();
    ASSERT_EQ(b.get_piece_at(8).get_type(), piece::none);
}
TEST(BoardTest, clear_board_empty) {
    board b = board();
    b.clear_board();
    for (int i = 0; i < 64; ++i) {
        ASSERT_EQ(b.get_piece_at(i).get_type(), piece::none);
    }
}