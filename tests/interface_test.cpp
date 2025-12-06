// interface_test.cpp
#include "constants.h"
#include <board.h>
#include <gtest/gtest.h>
#include <notation_interface.h>
#include <string>
using namespace pieces;
// Test that FEN for starting board is read correctly.
TEST(FEN_TEST, starting_board) {
    Board board;
    bool success = board.read_fen(NotationInterface::starting_FEN());
    assert(success);

    ASSERT_EQ(board.get_check(), 0);
    ASSERT_EQ(board.get_castling(), 0b1111);
    ASSERT_EQ(board.get_ply_moves(), 0);
    ASSERT_EQ(board.get_full_moves(), 1);
    ASSERT_EQ(board.get_turn_color(), white);
    assert(!board.get_en_passant());

    uint8_t row = 1;
    for (uint8_t col = 0; col < 8; col++) {
        uint8_t sq = NotationInterface::idx(row, col);
        ASSERT_EQ(board.get_piece_at(sq), Piece('P'));
    }
    row = 6;
    for (uint8_t col = 0; col < 8; col++) {
        uint8_t sq = NotationInterface::idx(row, col);
        ASSERT_EQ(board.get_piece_at(sq), Piece('p'));
    }

    ASSERT_EQ(board.get_piece_at(0), Piece('R'));
    ASSERT_EQ(board.get_piece_at(1), Piece('N'));
    ASSERT_EQ(board.get_piece_at(2), Piece('B'));
    ASSERT_EQ(board.get_piece_at(3), Piece('Q'));
    ASSERT_EQ(board.get_piece_at(4), Piece('K'));
    ASSERT_EQ(board.get_piece_at(5), Piece('B'));
    ASSERT_EQ(board.get_piece_at(6), Piece('N'));
    ASSERT_EQ(board.get_piece_at(7), Piece('R'));

    ASSERT_EQ(board.get_piece_at(56), Piece('r'));
    ASSERT_EQ(board.get_piece_at(57), Piece('n'));
    ASSERT_EQ(board.get_piece_at(58), Piece('b'));
    ASSERT_EQ(board.get_piece_at(59), Piece('q'));
    ASSERT_EQ(board.get_piece_at(60), Piece('k'));
    ASSERT_EQ(board.get_piece_at(61), Piece('b'));
    ASSERT_EQ(board.get_piece_at(62), Piece('n'));
    ASSERT_EQ(board.get_piece_at(63), Piece('r'));
}

TEST(FEN_TEST, get_fen) {
    std::string fen = NotationInterface::starting_FEN();
    Board board;
    bool success = board.read_fen(fen);
    assert(success);

    std::string fen_out = board.fen_from_state();

    ASSERT_EQ(fen, fen_out);

    fen = "rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2";
    success = board.read_fen(fen);
    assert(success);
    fen_out = board.fen_from_state();
    ASSERT_EQ(fen, fen_out);

    fen = "r2qkb1r/pp3npp/2p2p2/4p3/4P3/1P4NP/1PQP1PP1/2R1NRK1 b kq - 0 15";
    success = board.read_fen(fen);
    assert(success);
    fen_out = board.fen_from_state();
    ASSERT_EQ(fen, fen_out);
}
