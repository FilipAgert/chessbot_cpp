// board_test.cpp
#include "constants.h"
#include <board_state.h>
#include <notation_interface.h>
#include <integer_representation.h>
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
    state.read_fen(starting_fen);
    
    Move move;
    move.start_square = NotationInterface::idx_from_string("a2");
    move.end_square = NotationInterface::idx_from_string("a4");
    state.do_move(move);
    ASSERT_TRUE(state.board.is_square_empty(move.start_square));
    ASSERT_FALSE(state.board.is_square_empty(move.end_square));
    Piece p = Piece('P');
    ASSERT_EQ(state.board.get_piece_at(move.end_square), p);
    
    ASSERT_EQ(state.turn_color, black) ;
    state.undo_move(move);
    ASSERT_FALSE(state.board.is_square_empty(move.start_square));
    ASSERT_TRUE(state.board.is_square_empty(move.end_square));
    ASSERT_EQ(state.turn_color, white);
    
    state.do_move(move);

}

TEST(BoardStateTest, doUndoMovePromotion) {
    BoardState state;
    // White pawn on a7 ready to promote
    std::string fen = "8/P7/8/8/8/8/8/8 w - - 0 1";
    state.read_fen(fen);

    Move move;
    move.start_square = NotationInterface::idx_from_string("a7");
    move.end_square = NotationInterface::idx_from_string("a8");
    move.promotion = Piece('Q');

    state.do_move(move);

    ASSERT_TRUE(state.board.is_square_empty(move.start_square));
    ASSERT_EQ(state.board.get_piece_at(move.end_square),
              Piece('Q'));
    ASSERT_EQ(state.turn_color, black);

    state.undo_move(move);

    ASSERT_FALSE(state.board.is_square_empty(move.start_square));
    ASSERT_TRUE(state.board.is_square_empty(move.end_square));
    ASSERT_EQ(state.board.get_piece_at(move.start_square),
              Piece('P'));
    ASSERT_EQ(state.turn_color, white);
}
TEST(BoardStateTest, doUndoMoveCapture) {
    BoardState original;
    BoardState modified;
    // White pawn on e4, black pawn on d5 so exd5 is legal
    std::string fen = "8/8/8/3p4/4P3/8/8/8 w - - 0 1";
    modified.read_fen(fen);
    original.read_fen(fen);

    Move move;
    move.start_square = NotationInterface::idx_from_string("e4");
    move.end_square = NotationInterface::idx_from_string("d5");

    Piece captured = modified.board.get_piece_at(move.end_square);
    modified.do_move(move);

    ASSERT_EQ(modified.board.get_piece_at(move.end_square),
              Piece('P'));
    ASSERT_TRUE(modified.board.is_square_empty(move.start_square));
    ASSERT_EQ(modified.turn_color, black);

    modified.undo_move(move);

    ASSERT_EQ(modified.board.get_piece_at(move.start_square),
              Piece('P'));
    ASSERT_EQ(modified.board.get_piece_at(move.end_square), captured);
    ASSERT_EQ(modified.turn_color, white);
    ASSERT_EQ((int)modified.num_pieces, (int)original.num_pieces);
    ASSERT_EQ(modified, original);
}
TEST(BoardStateTest, doUndoMoveEnPassant) {
    BoardState state;
    // White pawn on e5, black pawn on d7 ready for d5 enabling e.p.
    std::string fen = "8/8/8/3pP3/8/8/8/8 w - d6 0 1";
    state.read_fen(fen);
    Move ep_move;
    ep_move.start_square = NotationInterface::idx_from_string("e5");
    ep_move.end_square = NotationInterface::idx_from_string("d6");

    state.do_move(ep_move);

    ASSERT_TRUE(state.board.is_square_empty(NotationInterface::idx_from_string("e5")));
    ASSERT_EQ(state.board.get_piece_at(NotationInterface::idx_from_string("d6")),
              Piece('P'));
    ASSERT_TRUE(state.board.is_square_empty(NotationInterface::idx_from_string("d5")));
    ASSERT_EQ(state.turn_color, black);

    state.undo_move(ep_move);

    ASSERT_FALSE(state.board.is_square_empty(NotationInterface::idx_from_string("e5")));
    ASSERT_EQ(state.board.get_piece_at(NotationInterface::idx_from_string("d5")),
              Piece('p'));
    ASSERT_TRUE(state.board.is_square_empty(NotationInterface::idx_from_string("d6")));
    ASSERT_EQ(state.turn_color, white);
}
TEST(BoardStateTest, chainedMovesUndoEquality) {
    BoardState original;
    BoardState modified;

    std::string fen = NotationInterface::starting_FEN();
    original.read_fen(fen);
    modified.read_fen(fen);

    // Moves: 1. e2e4, 1... e7e5, 2. g1f3
    Move m1, m2, m3;

    m1.start_square = NotationInterface::idx_from_string("e2");
    m1.end_square = NotationInterface::idx_from_string("e4");

    m2.start_square = NotationInterface::idx_from_string("e7");
    m2.end_square = NotationInterface::idx_from_string("e5");

    m3.start_square = NotationInterface::idx_from_string("g1");
    m3.end_square = NotationInterface::idx_from_string("f3");

    modified.do_move(m1);
    modified.do_move(m2);
    modified.do_move(m3);

    // Now undo in reverse order
    modified.undo_move(m3);
    modified.undo_move(m2);
    modified.undo_move(m1);

    ASSERT_TRUE(modified == original);
}
TEST(BoardStateTest, chainedMoveCapture) {
    BoardState original;
    BoardState modified;

    // Position engineered so white can capture and later promote
    std::string fen = 
        "8/4p1k1/8/3P4/8/8/8/5K2 b - - 0 1";

    original.read_fen(fen);
    modified.read_fen(fen);


    Move m1, m2;

    m1 = Move("e7e6");
    m2 = Move("d5e6");
    modified.do_move(m1);
    modified.do_move(m2);

    // Undo moves in reverse
    modified.undo_move(m2);
    modified.undo_move(m1);

    // Should match the  exactly
    ASSERT_TRUE(modified == original);
}
TEST(BoardStateTest, chainedMovesCapturePromotionUndoEquality) {
    BoardState original;
    BoardState modified;

    // Position engineered so white can capture and later promote
    std::string fen = 
        "8/4p1k1/8/3P4/8/8/8/5K2 b - - 0 1";

    original.read_fen(fen);
    modified.read_fen(fen);


    Move m1, m2, m3, m4, m5, m6;

    m1 = Move("e7e5");
    m2 = Move("d5e6");
    m3 = Move("g7f6");
    m4 = Move("e6e7");
    m5 = Move("f6f7");
    m6 = Move("e7e8Q");

    // Apply moves
    modified.do_move(m1);
    modified.do_move(m2);
    modified.do_move(m3);
    modified.do_move(m4);
    modified.do_move(m5);
    modified.do_move(m6);

    // Undo moves in reverse
    modified.undo_move(m6);
    modified.undo_move(m5);
    modified.undo_move(m4);
    modified.undo_move(m3);
    modified.undo_move(m2);
    modified.undo_move(m1);

    // Should match the  exactly
    ASSERT_TRUE(modified == original);
}

TEST(BoardStateTest, enPassantSquare){
    BoardState state;
    std::string starting_fen = NotationInterface::starting_FEN();
    state.read_fen(starting_fen);
    
    Move move;
    move.start_square = NotationInterface::idx_from_string("a2");
    move.end_square = NotationInterface::idx_from_string("a4");
    state.do_move(move);
    ASSERT_EQ(state.en_passant_square, NotationInterface::idx_from_string("a3"));
    ASSERT_TRUE(state.en_passant);
    
    move.start_square = NotationInterface::idx_from_string("a7");
    move.end_square = NotationInterface::idx_from_string("a5");
    state.do_move(move);
    ASSERT_TRUE(state.en_passant);
    ASSERT_EQ(state.en_passant_square, NotationInterface::idx_from_string("a6"));
    
    move = Move("b7b6");
    state.do_move(move);
    ASSERT_FALSE(state.en_passant); 
}