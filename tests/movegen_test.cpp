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
