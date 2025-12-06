// Copyright 2025 Filip Agert
#include <algorithm>
#include <array>
#include <bitboard.h>
#include <board.h>
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
using namespace magic;
#define idx_from_string NotationInterface::idx_from_string
TEST(magic_test, rookmagic) {

    for (int i = 0; i < 64; i++) {
        uint64_t occmask = rook_occupancy_table[i];
        int m = BitBoard::bitcount(occmask);
        std::array<uint64_t, max_size> occvar = gen_occ_variation(occmask, m);
        std::array<uint64_t, max_size> atk = compute_atk_bbs(occvar, i, true);
        int nelems = 1 << m;
        for (int j = 0; j < nelems; j++) {
            if (atk[j] != get_rook_atk_bb(i, occvar[j])) {
                BitBoard::print_full(atk[j]);
                BitBoard::print_full(get_rook_atk_bb(i, occvar[j]));
                std::cout << "i: " << i << "\n";
                ASSERT_FALSE(true);
            }
        }
    }
}

TEST(Movegentest, mated) {
    Board board;
    std::string fen = "8/7k/8/1P2Q1B1/8/P1K5/8/1B6 b - - 165 83";
    board.read_fen(fen);
    std::array<Move, max_legal_moves> moves;
    size_t num_moves = board.get_moves(moves);

    size_t expected = 1;  // First position offers 20 legal moves;
    ASSERT_EQ(expected, num_moves)
        << "Expected number of moves is 1. Actual found moves is" << num_moves;
}

TEST(Movegentest, promo_BUG) {
    Board board;
    std::string fen = "8/1R3QP1/8/8/8/6PP/8/k3K3 w - - 1 1";
    board.read_fen(fen);
    std::array<Move, max_legal_moves> moves;
    size_t num_moves = board.get_moves(moves);

    size_t expected = 41;  // First position offers 20 legal moves;
    ASSERT_EQ(expected, num_moves)
        << "Expected number of moves is 41. Actual found moves is" << num_moves;
}
TEST(magic_test, get_rook_magic_idx) {  // test that the array indices work correctly.

    for (int i = 0; i < 64; i++) {
        uint64_t occmask = rook_occupancy_table[i];
        int m = BitBoard::bitcount(occmask);
        std::array<uint64_t, max_size> occvar = gen_occ_variation(occmask, m);
        int nelems = 1ULL << m;
        int offset = rook_magic_offsets[i];
        int sz = rook_magic_sizes[i];
        for (int j = 0; j < nelems; j++) {
            int key = get_rook_magic_idx(i, occvar[j]);

            ASSERT_GE(key, offset) << "sq:" << i << "\n";       // Assert key grt or equal offset
            ASSERT_LT(key, offset + sz) << "sq:" << i << "\n";  // Assert key LT offset
        }
    }
}
// Tests the keys to make sure they stay within their domain (offset - offset+size)
TEST(Movegentest, kiwipete) {
    std::vector<int> moves = {48, 2039, 97862, 4085603, 193690690};
    std::string starting_fen =
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 1 1";
    int max = 4;
    for (int i = 0; i < max; i++) {
        int num_moves = movegen_benchmark::gen_num_moves(starting_fen, i + 1);
        size_t expected = moves[i];
        ASSERT_EQ(expected, num_moves);
    }
}
TEST(Movegentest, p3) {
    std::vector<int> moves = {14, 191, 2812, 43238, 674624};
    std::string starting_fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
    int max = 5;
    for (int i = 0; i < max; i++) {
        int num_moves = movegen_benchmark::gen_num_moves(starting_fen, i + 1);
        size_t expected = moves[i];
        ASSERT_EQ(expected, num_moves);
    }
}
TEST(Movegentest, p4w) {
    std::vector<int> moves = {6, 264, 9467, 422333, 15833292};
    std::string starting_fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
    int max = 5;
    for (int i = 0; i < max; i++) {
        int num_moves = movegen_benchmark::gen_num_moves(starting_fen, i + 1);
        size_t expected = moves[i];
        ASSERT_EQ(expected, num_moves);
    }
}
TEST(Movegentest, p4b) {
    std::vector<int> moves = {6, 264, 9467, 422333, 15833292};
    std::string starting_fen = "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1";
    int max = 5;
    for (int i = 0; i < max; i++) {
        int num_moves = movegen_benchmark::gen_num_moves(starting_fen, i + 1);
        size_t expected = moves[i];
        ASSERT_EQ(expected, num_moves);
    }
}

TEST(Movegentest, p5) {
    std::vector<int> moves = {44, 1486, 62379, 2103487};
    std::string starting_fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
    int max = 4;
    for (int i = 0; i < max; i++) {
        int num_moves = movegen_benchmark::gen_num_moves(starting_fen, i + 1);
        size_t expected = moves[i];
        ASSERT_EQ(expected, num_moves);
    }
}
TEST(Movegentest, p6) {
    std::vector<int> moves = {46, 2079, 89890, 3894594};
    std::string starting_fen =
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";
    int max = 4;
    for (int i = 0; i < max; i++) {
        int num_moves = movegen_benchmark::gen_num_moves(starting_fen, i + 1);
        size_t expected = moves[i];
        ASSERT_EQ(expected, num_moves);
    }
}
TEST(Movegentest, gen_moves) {
    Board board;
    std::string starting_fen = NotationInterface::starting_FEN();
    board.read_fen(starting_fen);
    std::array<Move, max_legal_moves> moves;
    size_t num_moves = board.get_moves(moves);

    size_t expected = 20;  // First position offers 20 legal moves;
    ASSERT_EQ(expected, num_moves)
        << "Expected number of moves is 20. Actual found moves is" << num_moves;
}
TEST(Movegentest, capture_check) {
    // Fen: King move out of check or capture checker valid moves.
    Board board;
    std::string fen = "8/8/8/8/4r2R/8/8/R3K2R w KQ - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    std::vector<std::string> expected_moves = {
        // Castling: NONE allowed because King is in check.

        // King Escape Moves (e1 can't go to e2 or d1/f1 if they were attacked,
        // but here e4 rook only attacks file. so diagonals/horizontals OK)
        "e1d1", "e1f1", "e1d2", "e1f2",
        // Note: e1e2 is illegal (still on e-file)
        "h4e4"};

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i)
        generated_moves.push_back(moves_array[i].toString());
    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());
    ASSERT_EQ(expected_moves, generated_moves) << "Failed FEN: " << fen;
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
    Board board;
    std::string fen = "8/p7/8/8/8/8/8/8 b - - 0 1";
    board.read_fen(fen);
    std::array<Move, max_legal_moves> moves;
    size_t num_moves = board.get_moves(moves);
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
    Board board;
    std::string fen = "rnbqkbnr/pppppppp/8/8/P7/8/1PPPPPPP/RNBQKBNR b KQkq - 0 1";
    board.read_fen(fen);
    std::array<Move, max_legal_moves> moves;
    size_t num_moves = board.get_moves(moves);
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

// 1.1 Castling disallowed (White, FEN Flag Not Present)
TEST(castle_test, Disallowed_White_NoFlag) {
    Board board;
    std::string fen = "r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1";
    board.read_fen(fen);
    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    std::vector<std::string> expected_moves = {
        // King moves (e1)
        "e1d1", "e1f1", "e1d2", "e1e2", "e1f2",
        // Rook moves (a1)
        "a1b1", "a1c1", "a1d1", "a1a2", "a1a3", "a1a4", "a1a5", "a1a6", "a1a7", "a1a8",
        // Rook moves (h1)
        "h1f1", "h1g1", "h1h2", "h1h3", "h1h4", "h1h5", "h1h6", "h1h7", "h1h8"};

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i) {
        generated_moves.push_back(moves_array[i].toString());
    }

    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());

    ASSERT_EQ(expected_moves.size(), num_moves);
    ASSERT_EQ(expected_moves, generated_moves)
        << "The generated moves do not match the expected moves for FEN: " << fen;
}
TEST(castle_test, Allowed_White_Kingside_Queenside) {
    // FEN: Full castling rights for White, empty board for simplicity
    Board board;
    std::string fen = "8/8/8/8/8/8/8/R3K2R w KQ - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    // Expected moves include O-O (e1g1) and O-O-O (e1c1), plus King/Rook normal moves
    std::vector<std::string> expected_moves = {
        // Castling Moves
        "e1g1",  // White Kingside (O-O)
        "e1c1",  // White Queenside (O-O-O)
        // King Moves
        "e1d1", "e1f1", "e1d2", "e1e2", "e1f2",
        // Rook moves (a1)
        "a1b1", "a1c1", "a1d1", "a1a2", "a1a3", "a1a4", "a1a5", "a1a6", "a1a7", "a1a8",
        // Rook moves (h1)
        "h1f1", "h1g1", "h1h2", "h1h3", "h1h4", "h1h5", "h1h6", "h1h7", "h1h8"};

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i) {
        generated_moves.push_back(moves_array[i].toString());
    }

    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());

    ASSERT_EQ(expected_moves.size(), num_moves);
    ASSERT_EQ(expected_moves, generated_moves)
        << "The generated moves do not match the expected moves for FEN: " << fen;
}
TEST(castle_test, Disallowed_White_Kingside_Blocked) {
    // FEN: Kingside path blocked by Knight on g1.
    Board board;
    std::string fen = "8/8/8/8/8/8/8/R3K1NR w KQ - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    std::vector<std::string> expected_moves = {
        // Castling: Only Queenside allowed (e1c1)
        "e1c1",
        // King Moves (g1 is occupied)
        "e1d1", "e1f1", "e1d2", "e1e2", "e1f2",
        // Rook a1 (Full range)
        "a1b1", "a1c1", "a1d1", "a1a2", "a1a3", "a1a4", "a1a5", "a1a6", "a1a7", "a1a8",
        // Rook h1 (Blocked by Knight at g1)
        "h1h2", "h1h3", "h1h4", "h1h5", "h1h6", "h1h7", "h1h8",
        // Knight g1 Moves
        "g1f3", "g1h3", "g1e2"};

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i)
        generated_moves.push_back(moves_array[i].toString());
    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());
    ASSERT_EQ(expected_moves, generated_moves) << "Failed FEN: " << fen;
}
TEST(castle_test, Disallowed_White_Queenside_Blocked1) {
    // FEN: Queenside path blocked by a Knight on c1. White has both flags.
    Board board;
    std::string fen = "8/8/8/8/8/8/8/R1N1K2R w KQ - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    // Expected: O-O (e1g1) is allowed. O-O-O (e1c1) is NOT allowed.
    std::vector<std::string> expected_moves = {
        // Castling Move (Kingside only)
        "e1g1",

        // King Moves (e1)
        "e1d1", "e1f1", "e1d2", "e1e2", "e1f2",

        // Rook Moves (a1) - Blocked horizontally past c1, but free vertically
        "a1b1",  // Path stops at c1
        "a1a2", "a1a3", "a1a4", "a1a5", "a1a6", "a1a7", "a1a8",

        // Rook Moves (h1) - Full range
        "h1g1", "h1f1", "h1h2", "h1h3", "h1h4", "h1h5", "h1h6", "h1h7", "h1h8",

        // Knight Moves (c1)
        "c1b3", "c1d3", "c1a2", "c1e2"  // Added c1a2, c1b2 for completeness
    };

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i) {
        generated_moves.push_back(moves_array[i].toString());
    }

    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());

    ASSERT_EQ(expected_moves.size(), num_moves);
    ASSERT_EQ(expected_moves, generated_moves)
        << "The generated moves do not match the expected moves for FEN: " << fen;
}
TEST(castle_test, Disallowed_White_Queenside_Blocked) {
    // FEN: Queenside path blocked by a Knight on c1. White has both flags.
    Board board;
    std::string fen = "8/8/8/8/8/8/8/RN2K2R w KQ - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    // Expected: O-O (e1g1) is allowed. O-O-O (e1c1) is NOT allowed.
    std::vector<std::string> expected_moves = {
        // Castling Move (Kingside only)
        "e1g1",

        // King Moves (e1)
        "e1d1", "e1f1", "e1d2", "e1e2", "e1f2",

        // Rook Moves (a1) - Blocked horizontally past c1, but free vertically
        "a1a2", "a1a3", "a1a4", "a1a5", "a1a6", "a1a7", "a1a8",

        // Rook Moves (h1) - Full range
        "h1g1", "h1f1", "h1h2", "h1h3", "h1h4", "h1h5", "h1h6", "h1h7", "h1h8",

        // Knight Moves (c1)
        "b1a3", "b1c3", "b1d2"  // Added c1a2, c1b2 for completeness
    };

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i) {
        generated_moves.push_back(moves_array[i].toString());
    }

    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());

    ASSERT_EQ(expected_moves.size(), num_moves);
    ASSERT_EQ(expected_moves, generated_moves)
        << "The generated moves do not match the expected moves for FEN: " << fen;
}
TEST(castle_test, Disallowed_Black_Kingside_Blocked) {
    // FEN: Black Kingside blocked by Knight on g8.
    Board board;
    std::string fen = "r3k1nr/8/8/8/8/8/8/8 b kq - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    std::vector<std::string> expected_moves = {
        // Castling: Only Queenside allowed
        "e8c8",
        // King Moves
        "e8d8", "e8f8", "e8d7", "e8e7", "e8f7",
        // Rook a8 (Full range)
        "a8b8", "a8c8", "a8d8", "a8a7", "a8a6", "a8a5", "a8a4", "a8a3", "a8a2", "a8a1",
        // Rook h8 (Blocked by Knight at g8)
        "h8h7", "h8h6", "h8h5", "h8h4", "h8h3", "h8h2", "h8h1",
        // Knight g8 Moves
        "g8f6", "g8h6", "g8e7"};

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i)
        generated_moves.push_back(moves_array[i].toString());
    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());
    ASSERT_EQ(expected_moves, generated_moves) << "Failed FEN: " << fen;
}
TEST(castle_test, Disallowed_Black_Queenside_Blocked) {
    // FEN: Black Queenside blocked by Knight on c8.
    Board board;
    std::string fen = "r1n1k2r/8/8/8/8/8/8/8 b kq - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    std::vector<std::string> expected_moves = {
        // Castling: Only Kingside allowed
        "e8g8",
        // King Moves
        "e8d8", "e8f8", "e8d7", "e8e7", "e8f7",
        // Rook h8 (Full range)
        "h8g8", "h8f8", "h8h7", "h8h6", "h8h5", "h8h4", "h8h3", "h8h2", "h8h1",
        // Rook a8 (Blocked at c8)
        "a8b8", "a8a7", "a8a6", "a8a5", "a8a4", "a8a3", "a8a2", "a8a1",
        // Knight c8 Moves
        "c8b6", "c8d6", "c8a7", "c8e7"};

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i)
        generated_moves.push_back(moves_array[i].toString());
    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());
    ASSERT_EQ(expected_moves, generated_moves) << "Failed FEN: " << fen;
}
TEST(castle_test, Disallowed_Black_Both_Blocked) {
    // FEN: Black Queenside blocked by knights next to king.
    Board board;
    std::string fen = "r2nkn1r/8/8/8/8/8/8/8 b kq - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    std::vector<std::string> expected_moves = {
        // Castling: Only Kingside allowed
        // King Moves
        "e8d7", "e8e7", "e8f7",
        // Rook h8 (Full range)
        "h8g8", "h8h7", "h8h6", "h8h5", "h8h4", "h8h3", "h8h2", "h8h1",
        // Rook a8 (Blocked at c8)
        "a8b8", "a8c8", "a8a7", "a8a6", "a8a5", "a8a4", "a8a3", "a8a2", "a8a1",
        // Knight d8 Moves
        "d8b7", "d8c6", "d8e6", "d8f7", "f8h7", "f8g6", "f8e6", "f8d7"};

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i)
        generated_moves.push_back(moves_array[i].toString());
    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());
    ASSERT_EQ(expected_moves, generated_moves) << "Failed FEN: " << fen;
}

TEST(castle_test, Disallowed_White_bothsides_ThruCheck1) {
    // FEN: Black Bishop on g4 attacks d1 (path for Queenside).
    Board board;
    std::string fen = "8/8/8/8/8/8/4b3/R3K2R w KQ - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    std::vector<std::string> expected_moves = {
        // King Moves (d1 is attacked, so King cannot move there normally either)
        "e1d2", "e1e2", "e1f2",
        // Rook a1 (Full)
        "a1b1", "a1c1", "a1d1", "a1a2", "a1a3", "a1a4", "a1a5", "a1a6", "a1a7", "a1a8",
        // Rook h1 (Full)
        "h1g1", "h1f1", "h1h2", "h1h3", "h1h4", "h1h5", "h1h6", "h1h7", "h1h8"};

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i)
        generated_moves.push_back(moves_array[i].toString());
    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());
    ASSERT_EQ(expected_moves, generated_moves) << "Failed FEN: " << fen;
}
TEST(castle_test, Disallowed_White_bothsides_ThruCheck2) {
    // FEN: Black Bishop on g4 attacks d1 (path for Queenside).
    Board board;
    std::string fen = "8/8/8/8/8/4b3/8/R3K2R w KQ - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    std::vector<std::string> expected_moves = {
        // King moves
        "e1e2", "e1d1", "e1f1",
        // Rook a1 (Full)
        "a1b1", "a1c1", "a1d1", "a1a2", "a1a3", "a1a4", "a1a5", "a1a6", "a1a7", "a1a8",
        // Rook h1 (Full)
        "h1g1", "h1f1", "h1h2", "h1h3", "h1h4", "h1h5", "h1h6", "h1h7", "h1h8"};

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i)
        generated_moves.push_back(moves_array[i].toString());
    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());
    ASSERT_EQ(expected_moves, generated_moves) << "Failed FEN: " << fen;
}
TEST(castle_test, allowed_White_bothsides_next2check) {
    // Should be allowed. check is after.
    Board board;
    std::string fen = "8/8/8/8/4b3/8/8/R3K2R w KQ - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    std::vector<std::string> expected_moves = {
        // King moves
        "e1e2", "e1d1", "e1f1", "e1d2", "e1f2",
        // Castles
        "e1g1", "e1c1",

        // Rook a1 (Full)
        "a1b1", "a1c1", "a1d1", "a1a2", "a1a3", "a1a4", "a1a5", "a1a6", "a1a7", "a1a8",
        // Rook h1 (Full)
        "h1g1", "h1f1", "h1h2", "h1h3", "h1h4", "h1h5", "h1h6", "h1h7", "h1h8"};

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i)
        generated_moves.push_back(moves_array[i].toString());
    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());
    ASSERT_EQ(expected_moves, generated_moves) << "Failed FEN: " << fen;
}
TEST(castle_test, Disallowed_Black_StartInCheck) {
    // FEN: Black King is in check from the White Rook on e5. Black has Kingside and queenside
    // rights.
    Board board;
    std::string fen = "r3k2r/8/8/4R3/8/8/8/8 b kq - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    // Expected: No O-O (e8g8). King must move to a safe square.
    std::vector<std::string> expected_moves = {// King Escape Moves
                                               "e8d7", "e8f7", "e8d8", "e8f8"};

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i) {
        generated_moves.push_back(moves_array[i].toString());
    }

    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());

    ASSERT_EQ(expected_moves.size(), num_moves);
    ASSERT_EQ(expected_moves, generated_moves)
        << "The generated moves do not match the expected moves for FEN: " << fen;
}
TEST(castle_test, Disallowed_White_ToCheck) {
    // FEN: White King has Kingside rights. Black Bishop on d4 attacks g1. King destination.
    Board board;
    std::string fen = "8/8/8/8/3b4/8/8/4K2R w K - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    // Expected: No O-O (e1g1). King can move to e2, d1, f1, etc.
    std::vector<std::string> expected_moves = {// King Moves (g1 is NOT included)
                                               "e1d1", "e1d2", "e1e2", "e1f1",

                                               // Rook Moves (h1) - Full range
                                               "h1g1", "h1f1", "h1h2", "h1h3", "h1h4", "h1h5",
                                               "h1h6", "h1h7", "h1h8"};

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i) {
        generated_moves.push_back(moves_array[i].toString());
    }

    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());

    ASSERT_EQ(expected_moves, generated_moves)
        << "The generated moves do not match the expected moves for FEN: " << fen;
}
TEST(castle_test, Disallowed_White_ThruCheck) {
    // FEN: White King has Kingside rights. Black Bishop on d4 attacks g1. King destination.
    Board board;
    std::string fen = "8/8/8/8/2b5/8/8/4K2R w K - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    // Expected: No O-O (e1g1). King can move to e2, d1, f1, etc.
    std::vector<std::string> expected_moves = {// King Moves (g1 is NOT included)
                                               "e1d1", "e1d2", "e1f2",

                                               // Rook Moves (h1) - Full range
                                               "h1g1", "h1f1", "h1h2", "h1h3", "h1h4", "h1h5",
                                               "h1h6", "h1h7", "h1h8"};

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i) {
        generated_moves.push_back(moves_array[i].toString());
    }

    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());

    ASSERT_EQ(expected_moves, generated_moves)
        << "The generated moves do not match the expected moves for FEN: " << fen;
}

TEST(castle_test, Disallowed_Black_both_ThruCheck1) {
    // FEN: White Bishop on c5 attacks f8.
    Board board;
    std::string fen = "r3k2r/4B3/8/8/8/8/8/8 b kq - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    std::vector<std::string> expected_moves = {
        // Castling: Queenside OK (e8c8). Kingside Disallowed.
        // King Moves (f8 is attacked)
        "e8d7", "e8e7", "e8f7",
        // Rook a8
        "a8b8", "a8c8", "a8d8", "a8a7", "a8a6", "a8a5", "a8a4", "a8a3", "a8a2", "a8a1",
        // Rook h8
        "h8g8", "h8f8", "h8h7", "h8h6", "h8h5", "h8h4", "h8h3", "h8h2", "h8h1"};

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i)
        generated_moves.push_back(moves_array[i].toString());
    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());
    ASSERT_EQ(expected_moves, generated_moves) << "Failed FEN: " << fen;
}
TEST(castle_test, Disallowed_Black_both_ThruCheck2) {
    // FEN: White Bishop on c5 attacks f8.
    Board board;
    std::string fen = "r3k2r/8/4B3/8/8/8/8/8 b kq - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    std::vector<std::string> expected_moves = {
        // Castling: Queenside OK (e8c8). Kingside Disallowed.
        // King Moves (f8 is attacked)
        "e8d8", "e8e7", "e8f8",
        // Rook a8
        "a8b8", "a8c8", "a8d8", "a8a7", "a8a6", "a8a5", "a8a4", "a8a3", "a8a2", "a8a1",
        // Rook h8
        "h8g8", "h8f8", "h8h7", "h8h6", "h8h5", "h8h4", "h8h3", "h8h2", "h8h1"};

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i)
        generated_moves.push_back(moves_array[i].toString());
    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());
    ASSERT_EQ(expected_moves, generated_moves) << "Failed FEN: " << fen;
}
TEST(castle_test, allowed_Black_both_enemyclose) {
    // FEN: White Bishop on c5 attacks f8.
    Board board;
    std::string fen = "r3k2r/8/8/4B3/8/8/8/8 b kq - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    std::vector<std::string> expected_moves = {
        "e8d8", "e8e7", "e8f8", "e8d7", "e8f7", "e8g8", "e8c8",
        // Rook a8
        "a8b8", "a8c8", "a8d8", "a8a7", "a8a6", "a8a5", "a8a4", "a8a3", "a8a2", "a8a1",
        // Rook h8
        "h8g8", "h8f8", "h8h7", "h8h6", "h8h5", "h8h4", "h8h3", "h8h2", "h8h1"};

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i)
        generated_moves.push_back(moves_array[i].toString());
    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());
    ASSERT_EQ(expected_moves, generated_moves) << "Failed FEN: " << fen;
}

TEST(castle_test, Disallowed_White_StartInCheck) {
    // FEN: White King is in check from Black Rook on e4.
    Board board;
    std::string fen = "8/8/8/8/4r3/8/8/R3K2R w KQ - 0 1";
    board.read_fen(fen);

    std::array<Move, max_legal_moves> moves_array;
    size_t num_moves = board.get_moves(moves_array);

    std::vector<std::string> expected_moves = {
        // Castling: NONE allowed because King is in check.

        // King Escape Moves (e1 can't go to e2 or d1/f1 if they were attacked,
        // but here e4 rook only attacks file. so diagonals/horizontals OK)
        "e1d1", "e1f1", "e1d2", "e1f2"
        // Note: e1e2 is illegal (still on e-file)
    };

    std::vector<std::string> generated_moves;
    for (size_t i = 0; i < num_moves; ++i)
        generated_moves.push_back(moves_array[i].toString());
    std::sort(expected_moves.begin(), expected_moves.end());
    std::sort(generated_moves.begin(), generated_moves.end());
    ASSERT_EQ(expected_moves, generated_moves) << "Failed FEN: " << fen;
}
TEST(castle_test, kingside_white) {
    Board board;
    std::string fen = "8/8/8/8/8/8/7P/4K2R w K - 0 1";
    board.read_fen(fen);
    std::array<Move, max_legal_moves> moves;
    size_t num_moves = board.get_moves(moves);
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
    ASSERT_EQ(board.get_castling(), 0b1);
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
