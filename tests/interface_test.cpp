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

TEST(FEN_TEST, get_fen) {
    std::string fen = NotationInterface::starting_FEN();
    BoardState state;
    bool success = NotationInterface::read_fen(fen, state);
    assert(success);

    std::string fen_out = NotationInterface::fen_from_state(state);

    ASSERT_EQ(fen, fen_out);

    fen = "rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2";
    success = NotationInterface::read_fen(fen, state);
    assert(success);
    fen_out = NotationInterface::fen_from_state(state);
    ASSERT_EQ(fen, fen_out);

    fen = "r2qkb1r/pp3npp/2p2p2/4p3/4P3/1P4NP/1PQP1PP1/2R1NRK1 b kq - 0 15";
    success = NotationInterface::read_fen(fen, state);
    assert(success);
    fen_out = NotationInterface::fen_from_state(state);
    ASSERT_EQ(fen, fen_out);

}