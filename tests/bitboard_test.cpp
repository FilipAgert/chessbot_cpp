
// board_test.cpp
#include <gtest/gtest.h>
#include <string>
#include <bitboard.h>
#include <notation_interface.h>
#include <cstdint>
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


TEST(BitBoardTest, test_knight_moves){
    uint64_t knight = 0b1;//Knight in a1. CAN attack:
    std::array<uint8_t, 64> attack;
    attack.fill(0); 
    attack[NotationInterface::idx_from_string("b3")] = 1;
    attack[NotationInterface::idx_from_string("c2")] = 1;
    uint64_t expected = BitBoard::bb_from_array(attack);
    
    uint64_t actual = BitBoard::knight_moves(knight);
    ASSERT_EQ(actual, expected);
    
    knight = 0x4000000000000000;
    attack.fill(0);
    attack[NotationInterface::idx_from_string("e7")] = 1;
    attack[NotationInterface::idx_from_string("f6")] = 1;
    attack[NotationInterface::idx_from_string("h6")] = 1;
    expected = BitBoard::bb_from_array(attack);
    actual = BitBoard::knight_moves(knight);
    ASSERT_EQ(actual, expected);

    knight = BitBoard::one_high(NotationInterface::idx_from_string("e4"));
    expected = BitBoard::bitcount(BitBoard::knight_moves(knight));
    actual = 8;
    ASSERT_EQ(expected, actual);

    

    knight = BitBoard::one_high(NotationInterface::idx_from_string("e4"));
    expected = BitBoard::bitcount(BitBoard::knight_moves(knight));
    actual = 8;
    ASSERT_EQ(expected, actual);

}