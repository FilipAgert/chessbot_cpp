
// board_test.cpp
#include <bitboard.h>
#include <cstdint>
#include <gtest/gtest.h>
#include <integer_representation.h>
#include <movegen.h>
#include <notation_interface.h>
#include <string>
using namespace BitBoard;
using namespace movegen;
using namespace dirs;
using namespace masks;
using namespace pieces;
TEST(BitBoardTest, onehigh) {
    uint8_t sq = 4;
    uint64_t bb = BitBoard::one_high(sq);
    uint64_t expected = 0b10000;
    ASSERT_EQ(bb, expected);

    sq = 63;
    bb = BitBoard::one_high(sq);
    expected = 0b1000000000000000000000000000000000000000000000000000000000000000;
    ASSERT_EQ(bb, expected) << ". Expected/actual:\n"
                            << BitBoard::bb_str(expected) << "\n"
                            << BitBoard::bb_str(bb) << "\n";
}
TEST(BitBoardTest, shift) {
    uint64_t bb = 0b1;
    uint64_t shifted = shift_bb(bb, N);
    uint64_t expected = 0b100000000;
    ASSERT_EQ(shifted, expected);

    bb = 0b10000000000000000000000000000000000;
    shifted = shift_bb(bb, SE);
    expected = bb >> 7;
    ASSERT_EQ(shifted, expected);
}

TEST(BitBoardTest, mask) {
    ASSERT_EQ(right, col(7));
    ASSERT_EQ(top, row(7));  // #
    ASSERT_EQ(sides, col(0) | col(7));
    ASSERT_EQ(top | bottom, row(0) | row(7));
}

TEST(BitBoardTest, test_knight_moves) {
    uint64_t knight = 0b1;  // Knight in a1. CAN attack:
    uint64_t f_bb = 0;
    std::array<uint8_t, 64> attack;
    attack.fill(0);
    attack[NotationInterface::idx_from_string("b3")] = 1;
    attack[NotationInterface::idx_from_string("c2")] = 1;
    uint64_t expected = BitBoard::bb_from_array(attack);

    uint64_t actual = movegen::knight_moves(knight, f_bb);
    ASSERT_EQ(actual, expected);

    knight = 0x4000000000000000;
    attack.fill(0);
    attack[NotationInterface::idx_from_string("e7")] = 1;
    attack[NotationInterface::idx_from_string("f6")] = 1;
    attack[NotationInterface::idx_from_string("h6")] = 1;
    expected = BitBoard::bb_from_array(attack);
    actual = movegen::knight_moves(knight, f_bb);
    ASSERT_EQ(actual, expected);

    attack[NotationInterface::idx_from_string("h6")] = 0;
    f_bb = BitBoard::one_high(NotationInterface::idx_from_string("h6"));
    expected = BitBoard::bb_from_array(attack);
    actual = movegen::knight_moves(knight, f_bb);
    ASSERT_EQ(actual, expected);

    knight = BitBoard::one_high(NotationInterface::idx_from_string("e4"));
    expected = BitBoard::bitcount(movegen::knight_moves(knight, f_bb));
    actual = 8;
    ASSERT_EQ(expected, actual);

    knight = BitBoard::one_high(NotationInterface::idx_from_string("e4"));
    expected = BitBoard::bitcount(movegen::knight_moves(knight, f_bb));
    actual = 8;
    ASSERT_EQ(expected, actual);
}
TEST(BitBoardTest, test_ray) {
    uint64_t origin = 0b1;  // A1.
    int dir = W;
    uint64_t attacked = movegen::ray(origin, dir);
    ASSERT_EQ(attacked, 0);
    origin = 0b10;  // A1.
    dir = W;
    attacked = movegen::ray(origin, dir);
    ASSERT_EQ(attacked, 0b1);

    dir = N;
    attacked = movegen::ray(origin, dir);
    ASSERT_EQ(attacked,
              masks::col(1) & ~BitBoard::one_high(NotationInterface::idx_from_string("b1")));

    dir = NW;
    attacked = movegen::ray(origin, dir);
    ASSERT_EQ(attacked, BitBoard::one_high(NotationInterface::idx_from_string("a2")));

    dir = NE;
    attacked = movegen::ray(origin, dir);
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
    attacked = movegen::ray(origin, dir, blockers);
    ASSERT_EQ(attacked, expected);
}

TEST(BitBoardTest, test_rook_moves) {
    // Rook on A1
    uint64_t rook = BitBoard::one_high(NotationInterface::idx_from_string("a1"));
    uint64_t f_bb = rook;
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
    uint64_t actual = movegen::rook_moves(rook, f_bb, e_bb);
    ASSERT_EQ(actual, expected);

    // Blockers: friendly piece on a4, enemy piece on d1
    f_bb = rook | BitBoard::one_high(NotationInterface::idx_from_string("a4"));
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
    actual = movegen::rook_moves(rook, f_bb, e_bb);
    ASSERT_EQ(actual, expected);
}

TEST(BitBoardTest, test_bishop_moves) {
    // Bishop on c1
    uint64_t bishop = BitBoard::one_high(NotationInterface::idx_from_string("c1"));
    uint64_t f_bb = bishop;
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
    uint64_t actual = movegen::bishop_moves(bishop, f_bb, e_bb);
    ASSERT_EQ(actual, expected);

    // Add enemy at f4, friendly at b2
    f_bb = bishop | BitBoard::one_high(NotationInterface::idx_from_string("b2"));
    e_bb = BitBoard::one_high(NotationInterface::idx_from_string("f4"));

    attack.fill(0);

    // up-left stops at friendly b2 (excluded)
    // up-right: d2,e3,f4 (enemy included)
    attack[NotationInterface::idx_from_string("d2")] = 1;
    attack[NotationInterface::idx_from_string("e3")] = 1;
    attack[NotationInterface::idx_from_string("f4")] = 1;

    expected = BitBoard::bb_from_array(attack);
    actual = movegen::bishop_moves(bishop, f_bb, e_bb);
    ASSERT_EQ(actual, expected);
}

TEST(BitBoardTest, test_queen_moves) {
    // Queen on d4
    uint64_t queen = BitBoard::one_high(NotationInterface::idx_from_string("d4"));
    uint64_t f_bb = queen;
    uint64_t e_bb = 0;

    // Expected = rook + bishop moves from d4
    std::array<uint8_t, 64> attack{};
    attack.fill(0);

    // rook directions
    for (auto sq :
         {"d1", "d2", "d3", "d5", "d6", "d7", "d8", "a4", "b4", "c4", "e4", "f4", "g4", "h4"})
        attack[NotationInterface::idx_from_string(sq)] = 1;

    // bishop directions
    for (auto sq : {"c3", "b2", "a1", "e5", "f6", "g7", "h8", "c5", "b6", "a7", "e3", "f2", "g1"})
        attack[NotationInterface::idx_from_string(sq)] = 1;

    uint64_t expected = BitBoard::bb_from_array(attack);
    uint64_t actual = movegen::queen_moves(queen, f_bb, e_bb);
    ASSERT_EQ(actual, expected);

    // Friendly at e4, enemy at f6
    f_bb = BitBoard::one_high(NotationInterface::idx_from_string("e4"));
    e_bb = BitBoard::one_high(NotationInterface::idx_from_string("f6"));

    attack.fill(0);

    // rook: stops at friendly e4 (excluded), includes left and up/down properly
    for (auto sq : {"d1", "d2", "d3", "d5", "d6", "d7", "d8", "a4", "b4", "c4"})
        attack[NotationInterface::idx_from_string(sq)] = 1;

    // bishop up-right: e5,f6 (enemy included)
    attack[NotationInterface::idx_from_string("e5")] = 1;
    attack[NotationInterface::idx_from_string("f6")] = 1;

    // bishop up-left: c5,b6,a7
    for (auto sq : {"c5", "b6", "a7"})
        attack[NotationInterface::idx_from_string(sq)] = 1;

    // bishop down-left: c3,b2,a1
    for (auto sq : {"c3", "b2", "a1"})
        attack[NotationInterface::idx_from_string(sq)] = 1;

    // bishop down-right: e3,f2,g1
    for (auto sq : {"e3", "f2", "g1"})
        attack[NotationInterface::idx_from_string(sq)] = 1;

    expected = BitBoard::bb_from_array(attack);
    actual = movegen::queen_moves(queen, f_bb, e_bb);
    ASSERT_EQ(actual, expected);
}

TEST(BitBoardTest, test_king_moves) {
    // King on e4, no blockers
    uint64_t king = BitBoard::one_high(NotationInterface::idx_from_string("e4"));
    uint64_t f_bb = king;

    std::array<uint8_t, 64> attack{};
    attack.fill(0);

    for (auto sq : {"e5", "f5", "f4", "f3", "e3", "d3", "d4", "d5"})
        attack[NotationInterface::idx_from_string(sq)] = 1;

    uint64_t expected = BitBoard::bb_from_array(attack);
    uint64_t actual = movegen::king_moves(king, f_bb);
    ASSERT_EQ(actual, expected);

    // Friendly on f4 blocks that move
    f_bb = king | BitBoard::one_high(NotationInterface::idx_from_string("f4"));

    attack[NotationInterface::idx_from_string("f4")] = 0;
    expected = BitBoard::bb_from_array(attack);
    actual = movegen::king_moves(king, f_bb);
    ASSERT_EQ(actual, expected);
}

TEST(BitBoardTest, test_pawn_moves) {
    // Utility array to build expected move bitboards
    std::array<uint8_t, 64> attack_map{};
    uint64_t expected;
    uint64_t actual;

    // --- SCENARIO 1: white PAWN - STARTING MOVES (1 & 2 squares) ---
    // Pawn on d2, no blockers. Expected: d3, d4.
    attack_map.fill(0);
    uint64_t pawn = BitBoard::one_high(NotationInterface::idx_from_string("d2"));
    uint64_t friendly = pawn;
    uint64_t enemy = 0;
    uint64_t ep = 0;

    for (auto sq : {"d3", "d4"})
        attack_map[NotationInterface::idx_from_string(sq)] = 1;

    expected = BitBoard::bb_from_array(attack_map);
    actual = pawn_moves(pawn, friendly, enemy, ep, white);
    ASSERT_EQ(actual, expected) << "White pawn on d2 failed 1/2 square move.";

    // --- SCENARIO 2: white PAWN - NORMAL 1-SQUARE MOVE ---
    // Pawn on d3, no blockers. Expected: d4. (Cannot move two squares)
    attack_map.fill(0);
    pawn = BitBoard::one_high(NotationInterface::idx_from_string("d3"));
    attack_map[NotationInterface::idx_from_string("d4")] = 1;

    expected = BitBoard::bb_from_array(attack_map);
    actual = pawn_moves(pawn, friendly, enemy, ep, white);
    ASSERT_EQ(actual, expected) << "White pawn on d3 failed 1 square move.";

    // --- SCENARIO 2: white PAWN - NORMAL 1-SQUARE BLOCKED ---
    // Pawn on d3, blockers. Expected: 0.
    attack_map.fill(0);
    pawn = BitBoard::one_high(NotationInterface::idx_from_string("d3"));
    friendly = pawn | BitBoard::one_high(NotationInterface::idx_from_string("d4"));
    attack_map[NotationInterface::idx_from_string("d4")] = 1;

    expected = 0;
    actual = pawn_moves(pawn, friendly, enemy, ep, white);
    ASSERT_EQ(actual, expected) << "White pawn on d3 failed to be blocked.";

    // --- SCENARIO 3: white PAWN - CAPTURES (Normal) ---
    // Pawn on e4. Enemy on d5 (capture) and e5 (block). Friendly on f5 (no move/capture).
    attack_map.fill(0);
    pawn = BitBoard::one_high(NotationInterface::idx_from_string("e4"));
    friendly = pawn | BitBoard::one_high(NotationInterface::idx_from_string("f5"));
    enemy = BitBoard::one_high(NotationInterface::idx_from_string("d5"));

    // Expected moves: capture on d5. Forward move e5. f5 is friendly.
    for (auto sq : {"d5", "e5"})
        attack_map[NotationInterface::idx_from_string(sq)] = 1;

    expected = BitBoard::bb_from_array(attack_map);
    actual = pawn_moves(pawn, friendly, enemy, ep, white);
    ASSERT_EQ(actual, expected) << "White pawn on e4 failed capture and blocking."
                                << BitBoard::bb_str(expected) << " " << BitBoard::bb_str(actual);

    // --- SCENARIO 4: white PAWN - EN PASSANT CAPTURE ---
    // Pawn on e5. Ep target square is f6 (i.e., black pawn moved f7-f5).
    // The pawn to capture is actually on f5, but the target is f6.
    attack_map.fill(0);
    pawn = BitBoard::one_high(NotationInterface::idx_from_string("e5"));
    friendly = pawn;
    enemy = 0;  // The captured enemy piece is assumed to be handled internally by ep_bb logic.
    ep = BitBoard::one_high(NotationInterface::idx_from_string("f6"));  // Ep target square

    attack_map[NotationInterface::idx_from_string("e6")] = 1;  // Normal forward move
    attack_map[NotationInterface::idx_from_string("f6")] = 1;  // En passant move

    expected = BitBoard::bb_from_array(attack_map);
    actual = pawn_moves(pawn, friendly, enemy, ep, white);
    ASSERT_EQ(actual, expected) << "White pawn on e5 failed En Passant move to f6."
                                << BitBoard::bb_str(actual);

    // --- SCENARIO 5: white PAWN - FORWARD BLOCKING (Friendly & Enemy) ---
    // Pawn on a2 (start rank). Blocked on a3 by friendly, a4 by enemy.
    attack_map.fill(0);
    pawn = BitBoard::one_high(NotationInterface::idx_from_string("a2"));
    friendly = pawn | BitBoard::one_high(NotationInterface::idx_from_string("a3"));
    enemy = BitBoard::one_high(NotationInterface::idx_from_string("a4"));
    ep = 0;

    // No expected moves, as a3 is blocked by friendly piece.
    expected = 0;
    actual = pawn_moves(pawn, friendly, enemy, ep, white);
    ASSERT_EQ(actual, expected) << "White pawn on a2 failed when blocked by friendly on a3.";

    // Pawn on a2. Blocked on a3 by enemy.
    friendly = pawn;
    enemy = BitBoard::one_high(NotationInterface::idx_from_string("a3"));

    // No expected moves, as a3 is blocked by enemy piece.
    expected = 0;
    actual = pawn_moves(pawn, friendly, enemy, ep, white);
    ASSERT_EQ(actual, expected) << "White pawn on a2 failed when blocked by enemy on a3.";

    // --- SCENARIO 6: white PAWN - NO WRAPAROUND (A and H files) ---
    // Pawn on a3. Enemy on b4. No wrap to h4.
    attack_map.fill(0);
    pawn = BitBoard::one_high(NotationInterface::idx_from_string("a3"));
    friendly = pawn;
    enemy = BitBoard::one_high(NotationInterface::idx_from_string("b4"));

    for (auto sq : {"a4", "b4"})  // Forward move a4, capture b4
        attack_map[NotationInterface::idx_from_string(sq)] = 1;

    expected = BitBoard::bb_from_array(attack_map);
    actual = pawn_moves(pawn, friendly, enemy, ep, white);
    ASSERT_EQ(actual, expected) << "White pawn on a3 failed A-file edge case/wraparound.";

    // Pawn on h3. Enemy on g4. No wrap to a4.
    attack_map.fill(0);
    pawn = BitBoard::one_high(NotationInterface::idx_from_string("h3"));
    friendly = pawn;
    enemy = BitBoard::one_high(NotationInterface::idx_from_string("g4"));

    for (auto sq : {"h4", "g4"})  // Forward move h4, capture g4
        attack_map[NotationInterface::idx_from_string(sq)] = 1;

    expected = BitBoard::bb_from_array(attack_map);
    actual = pawn_moves(pawn, friendly, enemy, ep, white);
    ASSERT_EQ(actual, expected) << "White pawn on h3 failed H-file edge case/wraparound.";

    // =================================================================
    // --- black PAWN TESTS (Symmetrical verification) ---
    // =================================================================

    // --- SCENARIO 7: black PAWN - STARTING MOVES (1 & 2 squares) ---
    // Pawn on d7, no blockers. Expected: d6, d5.
    attack_map.fill(0);
    pawn = BitBoard::one_high(NotationInterface::idx_from_string("d7"));
    friendly = pawn;
    enemy = 0;
    ep = 0;

    for (auto sq : {"d6", "d5"})
        attack_map[NotationInterface::idx_from_string(sq)] = 1;

    expected = BitBoard::bb_from_array(attack_map);
    actual = pawn_moves(pawn, friendly, enemy, ep, black);
    ASSERT_EQ(actual, expected) << "Black pawn on d7 failed 1/2 square move."
                                << BitBoard::bb_str(actual);

    // --- SCENARIO 8: black PAWN - CAPTUREST ---
    // Pawn on e4. Friendly on d3. enemy on f3.
    attack_map.fill(0);
    pawn = BitBoard::one_high(NotationInterface::idx_from_string("e4"));
    friendly = pawn | BitBoard::one_high(NotationInterface::idx_from_string(
                          "d3"));  // This spot is now a friendly to test no capture
    enemy = BitBoard::one_high(NotationInterface::idx_from_string("f3"));  // Normal capture
    ep = 0;

    // Expected moves: e3 (forward), f3 (capture).
    for (auto sq : {"e3", "f3"})
        attack_map[NotationInterface::idx_from_string(sq)] = 1;

    expected = BitBoard::bb_from_array(attack_map);
    actual = pawn_moves(pawn, friendly, enemy, ep, black);
    ASSERT_EQ(actual, expected) << "Black pawn on e4 failed mixed captures.";

    // --- SCENARIO 8: black PAWN - CAPTUREST ---
    // Pawn on e4. Friendly on d3. enemy on f3.
    attack_map.fill(0);
    pawn = BitBoard::one_high(NotationInterface::idx_from_string("e4"));
    friendly = pawn;
    enemy = BitBoard::one_high(NotationInterface::idx_from_string("d4")) |
            BitBoard::one_high(NotationInterface::idx_from_string("f3"));  // Normal capture
    ep = BitBoard::one_high(NotationInterface::idx_from_string("d3"));

    // Expected moves: e3 (forward), f3 (capture), d3 (ep capture).
    for (auto sq : {"e3", "f3", "d3"})
        attack_map[NotationInterface::idx_from_string(sq)] = 1;

    expected = BitBoard::bb_from_array(attack_map);
    actual = pawn_moves(pawn, friendly, enemy, ep, black);
    ASSERT_EQ(actual, expected) << "Black pawn on e4 failed mixed captures.";
    // --- SCENARIO 9: black PAWN - NO WRAPAROUND (H file) ---
    // Pawn on h6. Enemy on g5. No wrap to a5.
    attack_map.fill(0);
    pawn = BitBoard::one_high(NotationInterface::idx_from_string("h6"));
    friendly = pawn;
    enemy = BitBoard::one_high(NotationInterface::idx_from_string("g5"));

    for (auto sq : {"h5", "g5"})  // Forward move h5, capture g5
        attack_map[NotationInterface::idx_from_string(sq)] = 1;

    expected = BitBoard::bb_from_array(attack_map);
    actual = pawn_moves(pawn, friendly, enemy, ep, black);
    ASSERT_EQ(actual, expected) << "Black pawn on h6 failed H-file edge case/wraparound.";
}
