
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
    uint64_t f_bb=0;
    std::array<uint8_t, 64> attack;
    attack.fill(0); 
    attack[NotationInterface::idx_from_string("b3")] = 1;
    attack[NotationInterface::idx_from_string("c2")] = 1;
    uint64_t expected = BitBoard::bb_from_array(attack);
    
    uint64_t actual = BitBoard::knight_moves(knight,f_bb);
    ASSERT_EQ(actual, expected);
    
    knight = 0x4000000000000000;
    attack.fill(0);
    attack[NotationInterface::idx_from_string("e7")] = 1;
    attack[NotationInterface::idx_from_string("f6")] = 1;
    attack[NotationInterface::idx_from_string("h6")] = 1;
    expected = BitBoard::bb_from_array(attack);
    actual = BitBoard::knight_moves(knight, f_bb);
    ASSERT_EQ(actual, expected);

    attack[NotationInterface::idx_from_string("h6")] = 0;
    f_bb = BitBoard::one_high(NotationInterface::idx_from_string("h6"));
    expected = BitBoard::bb_from_array(attack);
    actual = BitBoard::knight_moves(knight, f_bb);
    ASSERT_EQ(actual, expected);

    knight = BitBoard::one_high(NotationInterface::idx_from_string("e4"));
    expected = BitBoard::bitcount(BitBoard::knight_moves(knight,f_bb));
    actual = 8;
    ASSERT_EQ(expected, actual);

    

    knight = BitBoard::one_high(NotationInterface::idx_from_string("e4"));
    expected = BitBoard::bitcount(BitBoard::knight_moves(knight, f_bb));
    actual = 8;
    ASSERT_EQ(expected, actual);

}
TEST(BitBoardTest, test_ray){
    uint64_t origin = 0b1;//A1.
    int dir = W;
    uint64_t attacked = BitBoard::ray(origin, dir);
    ASSERT_EQ(attacked, 0);
    origin = 0b10;//A1.
    dir = W;
    attacked = BitBoard::ray(origin, dir);
    ASSERT_EQ(attacked, 0b1);
    
    dir = N;
    attacked = BitBoard::ray(origin, dir);
    ASSERT_EQ(attacked, masks::col(1) & ~BitBoard::one_high(NotationInterface::idx_from_string("b1")));

    dir = NW;
    attacked = BitBoard::ray(origin, dir);
    ASSERT_EQ(attacked, BitBoard::one_high(NotationInterface::idx_from_string("a2")));

    dir = NE;
    attacked = BitBoard::ray(origin, dir);
    std::array<uint8_t, 64> attack;
    attack.fill(0); 
    attack[NotationInterface::idx_from_string("c2")] = 1;
    attack[NotationInterface::idx_from_string("d3")] = 1;
    attack[NotationInterface::idx_from_string("e4")] = 1;
    attack[NotationInterface::idx_from_string("f5")] = 1;
    attack[NotationInterface::idx_from_string("g6")] = 1;
    attack[NotationInterface::idx_from_string("h7")] = 1;
    uint64_t expected = BitBoard::bb_from_array(attack);
    ASSERT_EQ(attacked, expected);
    
    uint64_t blockers = BitBoard::one_high(NotationInterface::idx_from_string("f5"));
    attack[NotationInterface::idx_from_string("g6")] = 0;
    attack[NotationInterface::idx_from_string("h7")] = 0;
    expected = BitBoard::bb_from_array(attack);
    attacked = BitBoard::ray(origin, dir, blockers);
    ASSERT_EQ(attacked, expected);
}

TEST(BitBoardTest, test_rook_moves)
{
    // Rook on A1
    uint64_t rook = BitBoard::one_high(NotationInterface::idx_from_string("a1"));
    uint64_t f_bb = 0;
    uint64_t e_bb = 0;

    // Expected: all squares on file a (a2..a8) + all squares on rank 1 (b1..h1)
    std::array<uint8_t, 64> attack{};
    attack.fill(0);

    // vertical
    attack[NotationInterface::idx_from_string("a2")] = 1;
    attack[NotationInterface::idx_from_string("a3")] = 1;
    attack[NotationInterface::idx_from_string("a4")] = 1;
    attack[NotationInterface::idx_from_string("a5")] = 1;
    attack[NotationInterface::idx_from_string("a6")] = 1;
    attack[NotationInterface::idx_from_string("a7")] = 1;
    attack[NotationInterface::idx_from_string("a8")] = 1;

    // horizontal
    attack[NotationInterface::idx_from_string("b1")] = 1;
    attack[NotationInterface::idx_from_string("c1")] = 1;
    attack[NotationInterface::idx_from_string("d1")] = 1;
    attack[NotationInterface::idx_from_string("e1")] = 1;
    attack[NotationInterface::idx_from_string("f1")] = 1;
    attack[NotationInterface::idx_from_string("g1")] = 1;
    attack[NotationInterface::idx_from_string("h1")] = 1;

    uint64_t expected = BitBoard::bb_from_array(attack);
    uint64_t actual = BitBoard::rook_moves(rook, f_bb, e_bb);
    ASSERT_EQ(actual, expected);

    // Blockers: friendly piece on a4, enemy piece on d1
    f_bb = BitBoard::one_high(NotationInterface::idx_from_string("a4"));
    e_bb = BitBoard::one_high(NotationInterface::idx_from_string("d1"));

    attack.fill(0);

    // Up until blocker a4 (excluded)
    attack[NotationInterface::idx_from_string("a2")] = 1;
    attack[NotationInterface::idx_from_string("a3")] = 1;

    // Right until enemy on d1 (included)
    attack[NotationInterface::idx_from_string("b1")] = 1;
    attack[NotationInterface::idx_from_string("c1")] = 1;
    attack[NotationInterface::idx_from_string("d1")] = 1;

    expected = BitBoard::bb_from_array(attack);
    actual = BitBoard::rook_moves(rook, f_bb, e_bb);
    ASSERT_EQ(actual, expected);
}

TEST(BitBoardTest, test_bishop_moves)
{
    // Bishop on c1
    uint64_t bishop = BitBoard::one_high(NotationInterface::idx_from_string("c1"));
    uint64_t f_bb = 0;
    uint64_t e_bb = 0;

    std::array<uint8_t, 64> attack{};
    attack.fill(0);

    // Diagonal up-right (d2,e3,f4,g5,h6)
    attack[NotationInterface::idx_from_string("d2")] = 1;
    attack[NotationInterface::idx_from_string("e3")] = 1;
    attack[NotationInterface::idx_from_string("f4")] = 1;
    attack[NotationInterface::idx_from_string("g5")] = 1;
    attack[NotationInterface::idx_from_string("h6")] = 1;

    // Diagonal up-left (b2,a3)
    attack[NotationInterface::idx_from_string("b2")] = 1;
    attack[NotationInterface::idx_from_string("a3")] = 1;

    uint64_t expected = BitBoard::bb_from_array(attack);
    uint64_t actual = BitBoard::bishop_moves(bishop, f_bb, e_bb);
    ASSERT_EQ(actual, expected);

    // Add enemy at f4, friendly at b2
    f_bb = BitBoard::one_high(NotationInterface::idx_from_string("b2"));
    e_bb = BitBoard::one_high(NotationInterface::idx_from_string("f4"));

    attack.fill(0);

    // up-left stops at friendly b2 (excluded)
    // up-right: d2,e3,f4 (enemy included)
    attack[NotationInterface::idx_from_string("d2")] = 1;
    attack[NotationInterface::idx_from_string("e3")] = 1;
    attack[NotationInterface::idx_from_string("f4")] = 1;

    expected = BitBoard::bb_from_array(attack);
    actual = BitBoard::bishop_moves(bishop, f_bb, e_bb);
    ASSERT_EQ(actual, expected);
}

TEST(BitBoardTest, test_queen_moves)
{
    // Queen on d4
    uint64_t queen = BitBoard::one_high(NotationInterface::idx_from_string("d4"));
    uint64_t f_bb = 0;
    uint64_t e_bb = 0;

    // Expected = rook + bishop moves from d4
    std::array<uint8_t, 64> attack{};
    attack.fill(0);

    // rook directions
    for (auto sq : { "d1","d2","d3","d5","d6","d7","d8",
                     "a4","b4","c4","e4","f4","g4","h4" })
        attack[NotationInterface::idx_from_string(sq)] = 1;

    // bishop directions
    for (auto sq : { "c3","b2","a1","e5","f6","g7","h8",
                     "c5","b6","a7","e3","f2","g1" })
        attack[NotationInterface::idx_from_string(sq)] = 1;

    uint64_t expected = BitBoard::bb_from_array(attack);
    uint64_t actual = BitBoard::queen_moves(queen, f_bb, e_bb);
    ASSERT_EQ(actual, expected);

    // Friendly at e4, enemy at f6
    f_bb = BitBoard::one_high(NotationInterface::idx_from_string("e4"));
    e_bb = BitBoard::one_high(NotationInterface::idx_from_string("f6"));

    attack.fill(0);

    // rook: stops at friendly e4 (excluded), includes left and up/down properly
    for (auto sq : { "d1","d2","d3","d5","d6","d7","d8",
                     "a4","b4","c4" })
        attack[NotationInterface::idx_from_string(sq)] = 1;

    // bishop up-right: e5,f6 (enemy included)
    attack[NotationInterface::idx_from_string("e5")] = 1;
    attack[NotationInterface::idx_from_string("f6")] = 1;

    // bishop up-left: c5,b6,a7
    for (auto sq : { "c5","b6","a7" })
        attack[NotationInterface::idx_from_string(sq)] = 1;

    // bishop down-left: c3,b2,a1
    for (auto sq : { "c3","b2","a1" })
        attack[NotationInterface::idx_from_string(sq)] = 1;

    // bishop down-right: e3,f2,g1
    for (auto sq : { "e3","f2","g1" })
        attack[NotationInterface::idx_from_string(sq)] = 1;

    expected = BitBoard::bb_from_array(attack);
    actual = BitBoard::queen_moves(queen, f_bb, e_bb);
    ASSERT_EQ(actual, expected);
}

TEST(BitBoardTest, test_king_moves)
{
    // King on e4, no blockers
    uint64_t king = BitBoard::one_high(NotationInterface::idx_from_string("e4"));
    uint64_t f_bb = 0;

    std::array<uint8_t, 64> attack{};
    attack.fill(0);

    for (auto sq : { "e5","f5","f4","f3","e3","d3","d4","d5" })
        attack[NotationInterface::idx_from_string(sq)] = 1;

    uint64_t expected = BitBoard::bb_from_array(attack);
    uint64_t actual = BitBoard::king_moves(king, f_bb);
    ASSERT_EQ(actual, expected);

    // Friendly on f4 blocks that move
    f_bb = BitBoard::one_high(NotationInterface::idx_from_string("f4"));

    attack[NotationInterface::idx_from_string("f4")] = 0;
    expected = BitBoard::bb_from_array(attack);
    actual = BitBoard::king_moves(king, f_bb);
    ASSERT_EQ(actual, expected);
}
