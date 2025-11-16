// piece_test.cpp
#include "piece.h"
#include <gtest/gtest.h>
using namespace pieces;
// Test that `get_type()` correctly retrieves the type (binary masking)
TEST(PieceTest, GetType) {
    Piece p = Piece(pawn | white);
    ASSERT_EQ(p.get_type(), pawn);
}

// Test that `get_color()` correctly retrieves the color (binary masking)
TEST(PieceTest, GetColor) {
    Piece p = Piece(king | black);  // Set color to 'black'
    ASSERT_EQ(p.get_color(), black);
}

// Test that `get_string()` returns the correct character for a given value
TEST(PieceTest, GetString) {
    Piece p = Piece(white | knight);  // Set type to ' white knight'

    ASSERT_EQ(p.get_char(), 'N');  // 'P' is the character for the white pawn in all_piece_symbols
}

TEST(PieceTest, print_val) {
    Piece p = Piece(white | rook);  // Set type to 'white rook'
    
    testing::internal::CaptureStdout();  // Capture the output
    p.print_val();
    std::string output = testing::internal::GetCapturedStdout();  // Get the captured output

    ASSERT_EQ(output, "Piece value: 11\n");  // Check if the output is as expected
}

TEST(PieceTest, Piece_from_char) {
    Piece p = Piece('K');  // Create a Piece from character 'K'
    ASSERT_EQ(p.get_type(), king);  // Check if the type is 'king'
    ASSERT_EQ(p.get_color(), white);  // Check if the color is 'white'
}
TEST(PieceTest, piece_col_from_char) {
    ASSERT_EQ(Piece::piece_col_from_char('K'), white);  // Check if 'K' is white
    ASSERT_EQ(Piece::piece_col_from_char('k'), black);  // Check if 'k' is black
    ASSERT_EQ(Piece::piece_col_from_char('X'), none);   // Check if 'X' is none
}
TEST(PieceTest, piece_type_from_char) {
    ASSERT_EQ(Piece::piece_type_from_char('K'), king);   // Check if 'K' is king
    ASSERT_EQ(Piece::piece_type_from_char('k'), king);   // Check if 'k' is none
    ASSERT_EQ(Piece::piece_type_from_char('X'), none);   // Check if 'X' is none
}
TEST(PieceTest, are_colors_equal) {
    Piece p1(white | rook);
    Piece p2(white | bishop);  // Set type to 'white bishop'
    
    ASSERT_TRUE(Piece::are_colors_equal(p1, p2));  // Check if colors are equal

    Piece p3(black | rook);
    ASSERT_FALSE(Piece::are_colors_equal(p1, p3));  // Check if colors are not equal
    ASSERT_FALSE(Piece::are_colors_equal(p2, p3));  // Check if colors are not equal
}
TEST(PieceTest, are_types_equal) {
    Piece p1(white | rook);
    Piece p2(white | bishop);  // Set type to 'white bishop'
    
    ASSERT_FALSE(Piece::are_types_equal(p1, p2));  // Check if types are not equal

    Piece p3(white | rook);
    ASSERT_TRUE(Piece::are_types_equal(p1, p3));  // Check if types are equal
}
TEST(PieceTest, is_valid_Piece) {
    Piece p1(white | rook);
    ASSERT_TRUE(Piece::is_valid_piece(p1));  // Check if p1 is valid

    Piece p2(none);
    ASSERT_FALSE(Piece::is_valid_piece(p2));  // Check if p2 is not valid
}
TEST(PieceTest, equality_operator) {
    Piece p1(white | rook);
    Piece p2(white | rook);
    Piece p3(black | bishop);  // Set type to 'black bishop'
    
    ASSERT_TRUE(p1 == p2);  // Check if p1 is equal to p2
    ASSERT_FALSE(p1 == p3);  // Check if p1 is not equal to p3
}
TEST(PieceTest, default_constructor) {
    Piece p;
    ASSERT_EQ(p.get_type(), none);  // Check if the default value is none
    ASSERT_EQ(p.get_color(), none);  // Check if the default color is none
}
TEST(PieceTest, constructor_with_value) {
    Piece p(white | knight);  // Set type to 'white knight'
    ASSERT_EQ(p.get_type(), knight);  // Check if the type is 'knight'
    ASSERT_EQ(p.get_color(), white);  // Check if the color is 'white'
}
TEST(PieceTest, set_value) {
    Piece p;
    p.set_value( (black | queen));  // Set type to 'black queen'
    
    ASSERT_EQ(p.get_type(), queen);  // Check if the type is 'queen'
    ASSERT_EQ(p.get_color(), black);  // Check if the color is 'black'
}
TEST(PieceTest, set_value_invalid) {
    Piece p(none);
    
    ASSERT_EQ(p.get_type(), none);  // Check if the type is 'none'
    ASSERT_EQ(p.get_color(), none);  // Check if the color is 'none'
}
