// Copyright 2025 Filip Agert
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
using namespace BitBoard;
using namespace movegen;
using namespace dirs;
using namespace masks;
using namespace pieces;
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
TEST(Movegentest, test_queen_moves) {
    std::string fen = "8/8/8/8/8/4q3/8/4K3 b - - 0 1";
    BoardState state;
    state.read_fen(fen);
    std::array<Move, max_legal_moves> moves;
    size_t num_moves = state.get_moves(moves);
    size_t expected = 14;
    uint64_t bb = movegen::queen_moves(state.board.get_bb(black | queen), state.board.get_bb(black),
                                       state.board.get_bb(white));
    BitBoard::print_full(bb);
    ASSERT_EQ(num_moves, expected);
}
TEST(Movegentest, two_deep) {
    movegen_benchmark bench;
    int depth = 1;
    std::string starting_fen = NotationInterface::starting_FEN();
    int num_moves = bench.gen_num_moves(starting_fen, depth);
    size_t expected = 20;  // First position offers 20 legal moves;
    ASSERT_EQ(expected, num_moves)
        << "Expected number of moves is 20. Actual found moves is" << num_moves;

    depth = 2;
    num_moves = bench.gen_num_moves(starting_fen, depth);
    expected = 20 * 20;
    ASSERT_EQ(expected, num_moves)
        << "Expected number of moves is 40. Actual found moves is" << num_moves;
}

TEST(Movegentest, three_deep) {
    movegen_benchmark bench;
    int depth = 3;
    std::string starting_fen = NotationInterface::starting_FEN();
    int num_moves = bench.gen_num_moves(starting_fen, depth);
    size_t expected = 8902;
    ASSERT_EQ(expected, num_moves)
        << "Expected number of moves is 8902. Actual found moves is" << num_moves;
}
