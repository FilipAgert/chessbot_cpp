
// board_test.cpp
#include <gtest/gtest.h>
#include <string>
#include <bitboard.h>
using namespace BitBoard;
using namespace dirs;
using namespace masks;
TEST(BitBoardTest, shift){
    uint64_t bb = 0b1;
    uint64_t shifted = shift_bb(bb, N);
    uint64_t expected = 0b100000000;
    ASSERT_EQ(shifted, expected);
    
    bb = 0b10000000000000000000000000000000000;
    shifted = shift_bb(bb,SE);
    expected = bb>>7;
    ASSERT_EQ(shifted, expected);
}

TEST(BitBoardTest, mask){
    ASSERT_EQ(right, col(7));
    ASSERT_EQ(top, row(7));//#
    ASSERT_EQ(sides, col(0)|col(7));
    ASSERT_EQ(top|bottom, row(0) | row(7));
}