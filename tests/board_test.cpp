// board_test.cpp
#include "constants.h"
#include <board.h>
#include <gtest/gtest.h>
#include <integer_representation.h>
#include <string>
using namespace pieces;
// Test that Board() initializes all squares to empty pieces
TEST(BoardTest, Initialization) {
    Board b = Board();
    for (int i = 0; i < 64; ++i) {
        ASSERT_EQ(b.get_piece_at(i).get_type(), none);
    }
}

TEST(BoardTest, AddPiece) {
    Board b = Board();
    Piece p = Piece(white | rook);
    b.add_piece(8, p);
    ASSERT_EQ(b.get_piece_at(8), p);
}

TEST(BoardTest, MovePiece) {
    Board b = Board();
    Piece p1 = Piece(white | rook);
    Piece p2 = Piece(black | knight);
    b.add_piece(8, p1);
    b.add_piece(16, p2);
    b.move_piece(8, 16);
    ASSERT_EQ(b.get_piece_at(16), p1);
    ASSERT_EQ(b.get_piece_at(8).get_type(), none);
}

TEST(BoardTest, RemovePiece) {
    Board b = Board();
    Piece p = Piece(white | rook);
    b.add_piece(8, p);
    b.remove_piece(8);
    ASSERT_EQ(b.get_piece_at(8).get_type(),none);

    b.remove_piece(9);
    ASSERT_EQ(b.get_piece_at(9), Piece());
}

TEST(BoardTest, idx){
    uint8_t row = 0;
    uint8_t col = 0; //A1
    uint8_t idx_val = NotationInterface::idx(row, col);
    ASSERT_EQ(idx_val, 0);
    row = 7; //H8
    col = 7;
    idx_val = NotationInterface::idx(row, col);
    ASSERT_EQ(idx_val, 63);
    row = 1; // A2
    col = 0;
    idx_val = NotationInterface::idx(row, col);
    ASSERT_EQ(idx_val, 8);
    row = 0; // B1
    col = 1;
    idx_val = NotationInterface::idx(row, col);
    ASSERT_EQ(idx_val, 1);

    col = 7; //H1
    row = 0;
    idx_val = NotationInterface::idx(row, col);
    ASSERT_EQ(idx_val, 7);



    col = 7;
    row = 4;
    idx_val = NotationInterface::idx(row, col);
    uint8_t c2, r2;
    NotationInterface::row_col(r2, c2, idx_val);
    ASSERT_EQ(col,c2);
    ASSERT_EQ(row,r2);
    col = 0;
    row = 7;
    idx_val = NotationInterface::idx(row, col);
    NotationInterface::row_col(r2, c2, idx_val);
    ASSERT_EQ(col,c2);
    ASSERT_EQ(row,r2);
    col = 0;
    row = 0;
    idx_val = NotationInterface::idx(row, col);
    NotationInterface::row_col(r2, c2, idx_val);
    ASSERT_EQ(col,c2);
    ASSERT_EQ(row,r2);
    col = 7;
    row = 7;
    idx_val = NotationInterface::idx(row, col);
    NotationInterface::row_col(r2, c2, idx_val);
    ASSERT_EQ(col,c2);
    ASSERT_EQ(row,r2);
    col = 3;
    row = 3;
    idx_val = NotationInterface::idx(row, col);
    NotationInterface::row_col(r2, c2, idx_val);
    ASSERT_EQ(col,c2);
    ASSERT_EQ(row,r2);
    
}


TEST(BoardTest, string_from_idx){
    uint8_t idx = 0;
    std::string sq = "a1";
    ASSERT_EQ(sq, NotationInterface::string_from_idx(idx));

    idx = 9;
    sq = "b2";
    ASSERT_EQ(sq, NotationInterface::string_from_idx(idx));

    idx = 8;
    sq = "a2";
    ASSERT_EQ(sq, NotationInterface::string_from_idx(idx));
    
    for (uint8_t i = 0; i < 64 ; i++){
        sq = NotationInterface::string_from_idx(i);
        idx = NotationInterface::idx_from_string(sq);
        ASSERT_EQ(idx, i);
    }

}

TEST(BoardTest, idx_from_string){
    std::string idx = "A1";
    uint8_t idx_val = NotationInterface::idx_from_string(idx);
    ASSERT_EQ(idx_val, 0);

    idx = "a2";
    idx_val = NotationInterface::idx_from_string(idx);
    ASSERT_EQ(idx_val, 8);

    idx = "H8";
    idx_val = NotationInterface::idx_from_string(idx);
    ASSERT_EQ(idx_val, 63);

    idx = "A8";
    idx_val = NotationInterface::idx_from_string(idx);
    ASSERT_EQ(idx_val, 56);


    idx = "H1";
    idx_val = NotationInterface::idx_from_string(idx);
    ASSERT_EQ(idx_val, 7);
    idx = "A9";
    idx_val = NotationInterface::idx_from_string(idx);
    ASSERT_EQ(idx_val, err_val8);
    idx = "I1";
    idx_val = NotationInterface::idx_from_string(idx);
    ASSERT_EQ(idx_val, err_val8);
    idx = "A0";
    idx_val = NotationInterface::idx_from_string(idx);
    ASSERT_EQ(idx_val, err_val8);
    idx = "A";
    idx_val = NotationInterface::idx_from_string(idx);
    ASSERT_EQ(idx_val, err_val8);
    idx = "1";
    idx_val = NotationInterface::idx_from_string(idx);
    ASSERT_EQ(idx_val, err_val8);
    idx = "A1A";
    idx_val = NotationInterface::idx_from_string(idx);
    ASSERT_EQ(idx_val, err_val8);
}

TEST(BoardTest, is_square_empty) {
    Board b = Board();
    Piece p = Piece(white | rook);
    b.add_piece(8, p);
    ASSERT_FALSE(b.is_square_empty(8));
    ASSERT_TRUE(b.is_square_empty(9));
}

TEST(BoardTest, get_square_color) {
    Board b = Board();
    Piece p1 = Piece(white | rook);
    Piece p2 = Piece(black | knight);
    b.add_piece(8, p1);
    b.add_piece(16, p2);
    ASSERT_EQ(b.get_square_color(8), white);
    ASSERT_EQ(b.get_square_color(16), black);
}

TEST(BoardTest, clear_board) {
    Board b = Board();
    Piece p = Piece(white | rook);
    b.add_piece(8, p);
    b.clear_board();
    ASSERT_EQ(b.get_piece_at(8).get_type(), none);
}
TEST(BoardTest, clear_board_empty) {
    Board b = Board();
    b.clear_board();
    for (int i = 0; i < 64; ++i) {
        ASSERT_EQ(b.get_piece_at(i).get_type(), none);
    }
}
