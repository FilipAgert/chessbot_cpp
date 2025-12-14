// board_test.cpp
#include "constants.h"
#include <board.h>
#include <gtest/gtest.h>
#include <integer_representation.h>
#include <move.h>
#include <notation_interface.h>
#include <string>

using namespace pieces;
TEST(BoardTest, doUndoMove) {
    Board board;
    std::string starting_fen = NotationInterface::starting_FEN();
    board.read_fen(starting_fen);

    Move move;
    move.source = NotationInterface::idx_from_string("a2");
    move.target = NotationInterface::idx_from_string("a4");
    ASSERT_EQ(board.get_turn_color(), white);
    restore_move_info info = board.do_move_no_flag<true>(move);
    ASSERT_TRUE(board.is_square_empty(move.source));
    ASSERT_FALSE(board.is_square_empty(move.target));
    Piece p = Piece('P');
    ASSERT_EQ(board.get_piece_at(move.target), p);
    ASSERT_EQ(board.get_turn_color(), black);
    board.undo_move<true>(info, move);
    ASSERT_FALSE(board.is_square_empty(move.source));
    ASSERT_TRUE(board.is_square_empty(move.target));
    ASSERT_EQ(board.get_turn_color(), white);
}

TEST(BoardTest, doUndoMovePromotion) {
    Board board;
    // White pawn on a7 ready to promote
    std::string fen = "8/P7/8/8/8/8/8/8 w - - 0 1";
    board.read_fen(fen);

    Move move = Move(NotationInterface::idx_from_string("a7"),
                     NotationInterface::idx_from_string("a8"), Piece('Q'));

    restore_move_info info = board.do_move_no_flag<true>(move);

    ASSERT_TRUE(board.is_square_empty(move.source));
    ASSERT_EQ(board.get_piece_at(move.target), Piece('Q'));
    ASSERT_EQ(board.get_turn_color(), black);

    board.undo_move<true>(info, move);

    ASSERT_FALSE(board.is_square_empty(move.source));
    ASSERT_TRUE(board.is_square_empty(move.target));
    ASSERT_EQ(board.get_piece_at(move.source), Piece('P'));
    ASSERT_EQ(board.get_turn_color(), white);
}
TEST(BoardTest, doUndoMoveCapture) {
    Board original;
    Board modified;
    // White pawn on e4, black pawn on d5 so exd5 is legal
    std::string fen = "8/8/8/3p4/4P3/8/8/8 w - - 0 1";
    modified.read_fen(fen);
    original.read_fen(fen);

    Move move;
    move.source = NotationInterface::idx_from_string("e4");
    move.target = NotationInterface::idx_from_string("d5");

    Piece captured = modified.get_piece_at(move.target);
    restore_move_info info = modified.do_move_no_flag<true>(move);

    ASSERT_EQ(modified.get_piece_at(move.target), Piece('P'));
    ASSERT_TRUE(modified.is_square_empty(move.source));
    ASSERT_EQ(modified.get_turn_color(), black);

    modified.undo_move<true>(info, move);

    ASSERT_EQ(modified.get_piece_at(move.source), Piece('P'));
    ASSERT_EQ(modified.get_piece_at(move.target), captured);
    ASSERT_EQ(modified.get_turn_color(), white);
    ASSERT_EQ((int)modified.get_num_pieces(), (int)original.get_num_pieces());
    ASSERT_EQ(modified, original);
}
TEST(BoardTest, doUndoMoveEnPassant) {
    Board original;
    Board modified;

    // Position engineered so white can capture and later promote
    std::string fen = "8/4p1k1/8/3P4/8/8/8/5K2 b - - 0 1";

    original.read_fen(fen);
    modified.read_fen(fen);

    Move m1, m2, m3, m4, m5, m6;

    m1 = Move("e7e5");
    m2 = Move("d5e6");
    restore_move_info info1 = modified.do_move_no_flag<false>(m1);
    restore_move_info info2 = modified.do_move_no_flag<true>(m2);
    ASSERT_EQ(modified.get_num_pieces(), 3);
    modified.undo_move<true>(info2, m2);
    modified.undo_move<false>(info1, m1);
    ASSERT_EQ(modified, original);
}
TEST(BoardTest, doUndoMoveEnPassantFEN) {
    Board board;
    // White pawn on e5, black pawn on d7 ready for d5 enabling e.p.
    std::string fen = "8/8/8/3pP3/8/8/8/8 w - d6 0 1";
    board.read_fen(fen);
    Move ep_move;
    ep_move.source = NotationInterface::idx_from_string("e5");
    ep_move.target = NotationInterface::idx_from_string("d6");

    restore_move_info info = board.do_move_no_flag<true>(ep_move);

    ASSERT_TRUE(board.is_square_empty(NotationInterface::idx_from_string("e5")));
    ASSERT_EQ(board.get_piece_at(NotationInterface::idx_from_string("d6")), Piece('P'));
    ASSERT_TRUE(board.is_square_empty(NotationInterface::idx_from_string("d5")));
    ASSERT_EQ(board.get_turn_color(), black);

    board.undo_move<true>(info, ep_move);

    ASSERT_FALSE(board.is_square_empty(NotationInterface::idx_from_string("e5")));
    ASSERT_EQ(board.get_piece_at(NotationInterface::idx_from_string("d5")), Piece('p'));
    ASSERT_TRUE(board.is_square_empty(NotationInterface::idx_from_string("d6")));
    ASSERT_EQ(board.get_turn_color(), white);
}
TEST(BoardTest, chainedMovesUndoEquality) {
    Board original;
    Board modified;

    std::string fen = NotationInterface::starting_FEN();
    original.read_fen(fen);
    modified.read_fen(fen);

    // Moves: 1. e2e4, 1... e7e5, 2. g1f3
    Move m1, m2, m3;

    m1.source = NotationInterface::idx_from_string("e2");
    m1.target = NotationInterface::idx_from_string("e4");

    m2.source = NotationInterface::idx_from_string("e7");
    m2.target = NotationInterface::idx_from_string("e5");

    m3.source = NotationInterface::idx_from_string("g1");
    m3.target = NotationInterface::idx_from_string("f3");

    auto info1 = modified.do_move_no_flag<true>(m1);
    auto info2 = modified.do_move_no_flag<false>(m2);
    auto info3 = modified.do_move_no_flag<true>(m3);

    // Now undo in reverse order
    modified.undo_move<true>(info3, m3);
    modified.undo_move<false>(info2, m2);
    modified.undo_move<true>(info1, m1);

    ASSERT_TRUE(modified == original);
}
TEST(BoardTest, chainedMoveCapture) {
    Board original;
    Board modified;

    // Position engineered so white can capture and later promote
    std::string fen = "8/4p1k1/8/3P4/8/8/8/5K2 b - - 0 1";

    original.read_fen(fen);
    modified.read_fen(fen);

    Move m1, m2;

    m1 = Move("e7e6");
    m2 = Move("d5e6");
    auto info1 = modified.do_move_no_flag<false>(m1);
    auto info2 = modified.do_move_no_flag<true>(m2);

    // Undo moves in reverse
    modified.undo_move<true>(info2, m2);
    modified.undo_move<false>(info1, m1);

    // Should match the  exactly
    ASSERT_TRUE(modified == original);
}
TEST(BoardTest, chainedMovesCapturePromotionUndoEquality) {
    Board original;
    Board modified;

    // Position engineered so white can capture and later promote
    std::string fen = "8/4p1k1/8/3P4/8/8/8/5K2 b - - 0 1";

    original.read_fen(fen);
    modified.read_fen(fen);

    Move m1, m2, m3, m4, m5, m6;

    m1 = Move("e7e5");
    m2 = Move("d5e6");
    m3 = Move("g7f6");
    m4 = Move("e6e7");
    m5 = Move("f6f7");
    m6 = Move("e7e8Q");

    auto info1 = modified.do_move_no_flag<false>(m1);
    auto info2 = modified.do_move_no_flag<true>(m2);
    auto info3 = modified.do_move_no_flag<false>(m3);
    auto info4 = modified.do_move_no_flag<true>(m4);
    auto info5 = modified.do_move_no_flag<false>(m5);
    auto info6 = modified.do_move_no_flag<true>(m6);

    modified.undo_move<true>(info6, m6);
    modified.undo_move<false>(info5, m5);
    modified.undo_move<true>(info4, m4);
    modified.undo_move<false>(info3, m3);
    modified.undo_move<true>(info2, m2);
    modified.undo_move<false>(info1, m1);

    // Should match the  exactly
    ASSERT_TRUE(modified == original);
}

TEST(BoardTest, enPassantSquare) {
    Board board;
    std::string starting_fen = NotationInterface::starting_FEN();
    board.read_fen(starting_fen);

    Move move;
    move.source = NotationInterface::idx_from_string("a2");
    move.target = NotationInterface::idx_from_string("a4");
    auto info = board.do_move_no_flag<true>(move);
    ASSERT_EQ(board.get_en_passant_square(), NotationInterface::idx_from_string("a3"));
    ASSERT_TRUE(board.get_en_passant());

    move.source = NotationInterface::idx_from_string("a7");
    move.target = NotationInterface::idx_from_string("a5");
    auto info2 = board.do_move_no_flag<false>(move);
    ASSERT_TRUE(board.get_en_passant());
    ASSERT_EQ(board.get_en_passant_square(), NotationInterface::idx_from_string("a6"));

    move = Move("b7b6");
    auto info3 = board.do_move_no_flag<true>(move);
    ASSERT_FALSE(board.get_en_passant());
}
