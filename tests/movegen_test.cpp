// Copyright 2025 Filip Agert
#include <algorithm>
#include <array>
#include <bitboard.h>
#include <board_state.h>
#include <constants.h>
#include <cstdint>
#include <gtest/gtest.h>
#include <integer_representation.h>
#include <move.h>
#include <movegen.h>
#include <movegen_benchmark.h>
#include <notation_interface.h>
#include <string>
#include <vector>
using namespace BitBoard;
using namespace movegen;
using namespace dirs;
using namespace masks;
using namespace pieces;
#define idx_from_string NotationInterface::idx_from_string
TEST(Movegentest, gen_moves) {
    BoardState state;
    std::string starting_fen = NotationInterface::starting_FEN();
    state.read_fen(starting_fen);
    std::array<Move, max_legal_moves> moves;
    size_t num_moves = state.get_moves(moves);

    size_t expected = 20;  // First position offers 20 legal moves;
    ASSERT_EQ(expected, num_moves)
        << "Expected number of moves is 20. Actual found moves is" << num_moves;
}
TEST(Movegentest, two_deep) {
    int depth = 1;
    std::string starting_fen = NotationInterface::starting_FEN();
    int num_moves = movegen_benchmark::gen_num_moves(starting_fen, depth);
    size_t expected = 20;  // First position offers 20 legal moves;
    ASSERT_EQ(expected, num_moves)
        << "Expected number of moves is 20. Actual found moves is" << num_moves;

    depth = 2;
    num_moves = movegen_benchmark::gen_num_moves(starting_fen, depth);
    expected = 20 * 20;
    ASSERT_EQ(expected, num_moves)
        << "Expected number of moves is 40. Actual found moves is" << num_moves;
}
TEST(bbgentest, white_pawns) {
    uint64_t w_pawn_bb = BitBoard::one_high(idx_from_string("a2"));
    uint64_t expected =
        BitBoard::one_high(idx_from_string("a3")) | BitBoard::one_high(idx_from_string("a4"));
    uint64_t actual = movegen::pawn_moves(w_pawn_bb, w_pawn_bb, 0, 0, white);

    ASSERT_EQ(expected, actual) << BitBoard::to_string_bb(actual) << " "
                                << BitBoard::to_string_bb(expected) << "\n";
}
TEST(bbgentest, black_pawns) {
    uint64_t b_pawn_bb = BitBoard::one_high(idx_from_string("a7"));
    uint64_t expected =
        BitBoard::one_high(idx_from_string("a6")) | BitBoard::one_high(idx_from_string("a5"));

    uint64_t actual = movegen::pawn_moves(b_pawn_bb, b_pawn_bb, 0, 0, black);
    ASSERT_EQ(expected, actual) << BitBoard::to_string_bb(actual) << " "
                                << BitBoard::to_string_bb(expected) << "\n";
}
TEST(bbgentest, black_pawns_fen) {
    BoardState state;
    std::string fen = "8/p7/8/8/8/8/8/8 b - - 0 1";
    state.read_fen(fen);
    std::array<Move, max_legal_moves> moves;
    size_t num_moves = state.get_moves(moves);
    std::vector<std::string> expected_moves = {"a7a6", "a7a5"};
    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i) {
        generated_moves.push_back(moves[i].toString());
    }

    // 3. Sort both lists for order-independent comparison
    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());

    // 4. Assert that the sorted lists are identical
    ASSERT_EQ(expected_moves, generated_moves)
        << "The generated moves do not match the expected moves for FEN: " << fen;
}
TEST(bbgentest, fen1) {
    BoardState state;
    std::string fen = "rnbqkbnr/pppppppp/8/8/P7/8/1PPPPPPP/RNBQKBNR b KQkq - 0 1";
    state.read_fen(fen);
    std::array<Move, max_legal_moves> moves;
    size_t num_moves = state.get_moves(moves);
    std::vector<std::string> expected_moves = {
        "a7a6", "b7b6", "c7c6", "d7d6", "e7e6", "f7f6", "g7g6", "h7h6", "a7a5", "b7b5",
        "c7c5", "d7d5", "e7e5", "f7f5", "g7g5", "h7h5", "b8a6", "b8c6", "g8f6", "g8h6"};
    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i) {
        generated_moves.push_back(moves[i].toString());
    }

    // 3. Sort both lists for order-independent comparison
    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());

    // 4. Assert that the sorted lists are identical
    ASSERT_EQ(expected_moves, generated_moves)
        << "The generated moves do not match the expected moves for FEN: " << fen;
}
TEST(Movegentest, castle) {
    BoardState state;
    std::string fen = "8/8/8/8/8/8/7P/4K2R w K - 0 1";
    state.read_fen(fen);
    std::array<Move, max_legal_moves> moves;
    size_t num_moves = state.get_moves(moves);
    std::vector<std::string> expected_moves = {"e1f1", "e1g1", "h1g1", "h1f1", "h2h3",
                                               "h2h4", "e1d1", "e1d2", "e1e2", "e1f2"};
    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i) {
        generated_moves.push_back(moves[i].toString());
    }

    // 3. Sort both lists for order-independent comparison
    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());

    // 4. Assert that the sorted lists are identical
    ASSERT_EQ(expected_moves, generated_moves)
        << "The generated moves do not match the expected moves for FEN: " << fen;
}
TEST(Movegentest, three_deep) {
    int depth = 3;
    std::string starting_fen = NotationInterface::starting_FEN();
    int num_moves = movegen_benchmark::gen_num_moves(starting_fen, depth);
    size_t expected = 8902;
    ASSERT_EQ(expected, num_moves)
        << "Expected number of moves is 8902. Actual found moves is" << num_moves;
}
TEST(Movegentest, four_deep) {
    int depth = 4;
    std::string starting_fen = NotationInterface::starting_FEN();
    int num_moves = movegen_benchmark::gen_num_moves(starting_fen, depth);
    size_t expected = 197281;
    ASSERT_EQ(expected, num_moves)
        << "Expected number of moves is 197281. Actual found moves is" << num_moves;
}
