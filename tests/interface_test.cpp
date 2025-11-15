// interface_test.cpp
#include "constants.h"
#include <notation_interface.h>
#include <board_state.h>
#include <gtest/gtest.h>
#include <string>

//Test that FEN for starting state is read correctly.
TEST(FEN_TEST, starting_state) {
    BoardState state;
    bool success = NotationInterface::read_fen(NotationInterface::starting_FEN(), state);
    assert(success);
    
    ASSERT_EQ(state.check, 0);
    ASSERT_EQ(state.castling, 0b1111);
    ASSERT_EQ(state.ply_moves, 0);
    ASSERT_EQ(state.full_moves, 1);
    ASSERT_EQ(state.turn_color, Piece::white);
    assert(!state.en_passant);



    uint8_t row = 1;
    for (uint8_t col = 0; col < 8; col++){
        uint8_t sq = Board::idx(row, col);
        ASSERT_EQ(state.board.get_piece_at(sq), Piece::piece_from_char('P'));
    }
    row = 6;
    for (uint8_t col = 0; col < 8; col++){
        uint8_t sq = Board::idx(row, col);
        ASSERT_EQ(state.board.get_piece_at(sq), Piece::piece_from_char('p'));
    }

    ASSERT_EQ(state.board.get_piece_at(0), Piece::piece_from_char('R'));
    ASSERT_EQ(state.board.get_piece_at(1), Piece::piece_from_char('N'));
    ASSERT_EQ(state.board.get_piece_at(2), Piece::piece_from_char('B'));
    ASSERT_EQ(state.board.get_piece_at(3), Piece::piece_from_char('Q'));
    ASSERT_EQ(state.board.get_piece_at(4), Piece::piece_from_char('K'));
    ASSERT_EQ(state.board.get_piece_at(5), Piece::piece_from_char('B'));
    ASSERT_EQ(state.board.get_piece_at(6), Piece::piece_from_char('N'));
    ASSERT_EQ(state.board.get_piece_at(7), Piece::piece_from_char('R'));

    ASSERT_EQ(state.board.get_piece_at(56), Piece::piece_from_char('r'));
    ASSERT_EQ(state.board.get_piece_at(57), Piece::piece_from_char('n'));
    ASSERT_EQ(state.board.get_piece_at(58), Piece::piece_from_char('b'));
    ASSERT_EQ(state.board.get_piece_at(59), Piece::piece_from_char('q'));
    ASSERT_EQ(state.board.get_piece_at(60), Piece::piece_from_char('k'));
    ASSERT_EQ(state.board.get_piece_at(61), Piece::piece_from_char('b'));
    ASSERT_EQ(state.board.get_piece_at(62), Piece::piece_from_char('n'));
    ASSERT_EQ(state.board.get_piece_at(63), Piece::piece_from_char('r'));
}