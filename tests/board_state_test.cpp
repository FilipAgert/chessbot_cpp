// board_test.cpp
#include "constants.h"
#include <board_state.h>
#include <notation_interface.h>
#include <gtest/gtest.h>
#include <string>
#include <move.h>

// Test that Board() initializes all squares to empty pieces
TEST(BoardStateTest, shift_piece_loc){
    BoardState state;
    state.piece_locations[0] = 16;
    state.piece_locations[1] = 6;
    state.piece_locations[2] = 50;
    state.piece_locations[3] = 58;
    state.num_pieces = 4;
    state.piece_loc_remove(6);
    
    ASSERT_EQ(state.piece_locations[0], 16);
    ASSERT_EQ(state.piece_locations[1], 50);
    ASSERT_EQ(state.piece_locations[2], 58);
    
}

TEST(BoardStateTest, doUndoMove){
    BoardState state;
    std::string starting_fen = NotationInterface::starting_FEN();
    NotationInterface::read_fen(starting_fen, state);
    
    Move move;
    move.start_square = Board::idx_from_string("a2");
    move.end_square = Board::idx_from_string("a4");
    state.do_move(move);
    ASSERT_TRUE(state.board.is_square_empty(move.start_square));
    ASSERT_FALSE(state.board.is_square_empty(move.end_square));
    ASSERT_EQ(state.board.get_piece_at(move.end_square), Piece::piece_from_char('P'));
    ASSERT_EQ(state.turn_color, Piece::black) ;
    state.undo_move(move);
    ASSERT_FALSE(state.board.is_square_empty(move.start_square));
    ASSERT_TRUE(state.board.is_square_empty(move.end_square));
    ASSERT_EQ(state.turn_color, Piece::white);
    
    state.do_move(move);

}

TEST(BoardStateTest, doUndoMovePromotion) {
    BoardState state;
    // White pawn on a7 ready to promote
    std::string fen = "8/P7/8/8/8/8/8/8 w - - 0 1";
    NotationInterface::read_fen(fen, state);

    Move move;
    move.start_square = Board::idx_from_string("a7");
    move.end_square = Board::idx_from_string("a8");
    move.promotion = Piece::piece_from_char('Q');

    state.do_move(move);

    ASSERT_TRUE(state.board.is_square_empty(move.start_square));
    ASSERT_EQ(state.board.get_piece_at(move.end_square),
              Piece::piece_from_char('Q'));
    ASSERT_EQ(state.turn_color, Piece::black);

    state.undo_move(move);

    ASSERT_FALSE(state.board.is_square_empty(move.start_square));
    ASSERT_TRUE(state.board.is_square_empty(move.end_square));
    ASSERT_EQ(state.board.get_piece_at(move.start_square),
              Piece::piece_from_char('P'));
    ASSERT_EQ(state.turn_color, Piece::white);
}
