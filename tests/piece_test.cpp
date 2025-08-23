// piece_test.cpp
#include "piece.h"
#include <gtest/gtest.h>

// Test that `get_type()` correctly retrieves the type (binary masking)
TEST(PieceTest, GetType) {
    piece p;
    p.value = (piece::pawn | piece::white);  // Set type to 'pawn'
    ASSERT_EQ(p.get_type(), piece::pawn);
}

// Test that `get_color()` correctly retrieves the color (binary masking)
TEST(PieceTest, GetColor) {
    piece p;
    p.value = (piece::king | piece::black);  // Set color to 'black'
    ASSERT_EQ(p.get_color(), piece::black);
}

// Test that `get_string()` returns the correct character for a given value
TEST(PieceTest, GetString) {
    piece p;
    p.value = (piece::white | piece::knight);  // Set type to ' white knight'

    ASSERT_EQ(p.get_string(), 'N');  // 'P' is the character for the white pawn in all_piece_symbols
}

TEST(PieceTest, print_val) {
    piece p;
    p.value = (piece::white | piece::rook);  // Set type to 'white rook'
    
    testing::internal::CaptureStdout();  // Capture the output
    p.print_val();
    std::string output = testing::internal::GetCapturedStdout();  // Get the captured output

    ASSERT_EQ(output, "Piece value: 11\n");  // Check if the output is as expected
}

TEST(PieceTest, piece_from_char) {
    piece p = piece::piece_from_char('K');  // Create a piece from character 'K'
    ASSERT_EQ(p.get_type(), piece::king);  // Check if the type is 'king'
    ASSERT_EQ(p.get_color(), piece::white);  // Check if the color is 'white'
}
TEST(PieceTest, piece_col_from_char) {
    ASSERT_EQ(piece::piece_col_from_char('K'), piece::white);  // Check if 'K' is white
    ASSERT_EQ(piece::piece_col_from_char('k'), piece::black);  // Check if 'k' is black
    ASSERT_EQ(piece::piece_col_from_char('X'), piece::none);   // Check if 'X' is none
}
TEST(PieceTest, piece_type_from_char) {
    ASSERT_EQ(piece::piece_type_from_char('K'), piece::king);   // Check if 'K' is king
    ASSERT_EQ(piece::piece_type_from_char('k'), piece::king);   // Check if 'k' is none
    ASSERT_EQ(piece::piece_type_from_char('X'), piece::none);   // Check if 'X' is none
}
TEST(PieceTest, are_colors_equal) {
    piece p1(piece::white | piece::rook);
    piece p2(piece::white | piece::bishop);  // Set type to 'white bishop'
    
    ASSERT_TRUE(piece::are_colors_equal(p1, p2));  // Check if colors are equal

    piece p3(piece::black | piece::rook);
    ASSERT_FALSE(piece::are_colors_equal(p1, p3));  // Check if colors are not equal
    ASSERT_FALSE(piece::are_colors_equal(p2, p3));  // Check if colors are not equal
}
TEST(PieceTest, are_types_equal) {
    piece p1(piece::white | piece::rook);
    piece p2(piece::white | piece::bishop);  // Set type to 'white bishop'
    
    ASSERT_FALSE(piece::are_types_equal(p1, p2));  // Check if types are not equal

    piece p3(piece::white | piece::rook);
    ASSERT_TRUE(piece::are_types_equal(p1, p3));  // Check if types are equal
}
TEST(PieceTest, is_valid_piece) {
    piece p1(piece::white | piece::rook);
    ASSERT_TRUE(piece::is_valid_piece(p1));  // Check if p1 is valid

    piece p2(piece::none);
    ASSERT_FALSE(piece::is_valid_piece(p2));  // Check if p2 is not valid
}
TEST(PieceTest, equality_operator) {
    piece p1(piece::white | piece::rook);
    piece p2(piece::white | piece::rook);
    piece p3(piece::black | piece::bishop);  // Set type to 'black bishop'
    
    ASSERT_TRUE(p1 == p2);  // Check if p1 is equal to p2
    ASSERT_FALSE(p1 == p3);  // Check if p1 is not equal to p3
}
TEST(PieceTest, default_constructor) {
    piece p;
    ASSERT_EQ(p.get_type(), piece::none);  // Check if the default value is none
    ASSERT_EQ(p.get_color(), piece::none);  // Check if the default color is none
}
TEST(PieceTest, constructor_with_value) {
    piece p(piece::white | piece::knight);  // Set type to 'white knight'
    ASSERT_EQ(p.get_type(), piece::knight);  // Check if the type is 'knight'
    ASSERT_EQ(p.get_color(), piece::white);  // Check if the color is 'white'
}
TEST(PieceTest, set_value) {
    piece p;
    p.value = (piece::black | piece::queen);  // Set type to 'black queen'
    
    ASSERT_EQ(p.get_type(), piece::queen);  // Check if the type is 'queen'
    ASSERT_EQ(p.get_color(), piece::black);  // Check if the color is 'black'
}
TEST(PieceTest, set_value_invalid) {
    piece p;
    p.value = (piece::none);  // Set type to 'none'
    
    ASSERT_EQ(p.get_type(), piece::none);  // Check if the type is 'none'
    ASSERT_EQ(p.get_color(), piece::none);  // Check if the color is 'none'
}