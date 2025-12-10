
#include <board.h>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <notation_interface.h>
#include <tables.h>
#include <moveorder.h>

TEST(ZobroistTest, rand) {
    int numhi = 0;
    int numtest = 10000;
    for (int i = 0; i < numtest; i++) {
        uint64_t number = ZobroistHasher::get().rand_uint64_t();
        numhi += BitBoard::bitcount(number);
    }

    int avg = numtest * 32;
    float stdev = sqrt(64 * numtest * 0.5 * 0.5);
    float nstdev = abs(numhi - avg) / stdev;

    float expected = 3;
    ASSERT_LE(nstdev, expected);
}
TEST(ZobroistTest, test_hash_nbr_grt0) {
    ASSERT_GT(ZobroistHasher::get().black_number, 0);

    for (std::array<uint64_t, 64> arr : ZobroistHasher::get().piece_numbers) {
        for (uint64_t n : arr) {
            assert(n > 0);
        }
    }
    for (uint64_t n : ZobroistHasher::get().castle_numbers) {
        assert(n > 0);
    }
    for (uint64_t n : ZobroistHasher::get().ep_numbers) {
        assert(n > 0);
    }
}

TEST(ZobroistTest, hashState) {
    Board state;
    state.read_fen(NotationInterface::starting_FEN());
    uint64_t hash = ZobroistHasher::get().hash_board(state);
    assert(hash > 0);
    int bitcount = BitBoard::bitcount(hash);

    assert(bitcount > 15);
    assert(bitcount < 64 - 15);
}

TEST(TransTest, optionalReturn) {
    Board state;
    state.read_fen(NotationInterface::starting_FEN());
    uint64_t hash = ZobroistHasher::get().hash_board(state);
    std::unique_ptr<transposition_table> tab = std::make_unique<transposition_table>();
    ASSERT_FALSE(tab->get(hash));
    tab->store(hash, Move{2, 9}, 0, transposition_entry::exact, 0);
    ASSERT_TRUE(tab->get(hash));
}
TEST(TransTest, moveOrderTest) {
    Board state;
    state.read_fen(NotationInterface::starting_FEN());
    Move bestmove = Move("a2a4");
    std::array<Move, max_legal_moves> moves;
    size_t nummoves = state.get_moves<normal_search>(moves);
    MoveOrder::apply_move_sort(moves, nummoves, bestmove, state); 
    ASSERT_TRUE(moves[0].source == bestmove.source && moves[0].target == bestmove.target);
    bestmove = Move("b2b4");
    MoveOrder::apply_move_sort(moves, nummoves, bestmove, state); 
    ASSERT_TRUE(moves[0].source == bestmove.source && moves[0].target == bestmove.target);
    for(int i = nummoves; i < max_legal_moves; i++){
        ASSERT_EQ(moves[i].source , err_val8);
        ASSERT_EQ(moves[i].target , err_val8);
    }
}
TEST(ZobroistTest, TranspositionIdentity) {
    // We will establish the position after 1. e4 e5 2. Nf3 Nc6 3. Nc3 Nf6

    // --- SETUP: Standard Move Order (A) ---
    Board board_A;
    board_A.read_fen(NotationInterface::starting_FEN());

    // Define moves using internal representation (e.g., from square, to square)
    // NOTE: You must replace the integer pairs with the correct representation
    //       used by your 'Move' constructor (e.g., (E2, E4) or a move object)
    
    // Example using placeholder squares (assuming 0-63 indices):
    // E2=12, E4=28, E7=52, E5=36, G1=6, F3=21, B8=57, C6=42, B1=1, C3=18, G8=62, F6=45

    std::vector<Move> moves_A = {
        Move{12, 28}, // 1. e4
        Move{52, 36}, // 1... e5
        Move{6,  21}, // 2. Nf3
        Move{57, 42}, // 2... Nc6
        Move{1,  18}, // 3. Nc3
        Move{62, 45}  // 3... Nf6
    };

    for (auto& move : moves_A) {
        board_A.do_move(move);
    }

    uint64_t hash_A = ZobroistHasher::get().hash_board(board_A);
    
    // --- SETUP: Swapped Move Order (B) ---
    Board board_B;
    board_B.read_fen(NotationInterface::starting_FEN());

    std::vector<Move> moves_B = {
        Move{12, 28}, // 1. e4
        Move{52, 36}, // 1... e5
        Move{1,  18}, // 2. Nc3  <-- Swapped
        Move{62, 45}, // 2... Nf6 <-- Swapped
        Move{6,  21}, // 3. Nf3
        Move{57, 42}  // 3... Nc6
    };

    for (auto& move : moves_B) {
        board_B.do_move(move);
    } 

    uint64_t hash_B = ZobroistHasher::get().hash_board(board_B);

    // --- ASSERTION ---
    // The final positions are identical, so their Zobrist hashes must be identical.
    ASSERT_EQ(hash_A, hash_B) << "Zobrist hash failed: Hashes for the same position are different!";
    
    // Optional: Print final FENs to visually confirm they are identical
    // std::cout << "FEN A: " << board_A.to_fen() << std::endl;
    // std::cout << "FEN B: " << board_B.to_fen() << std::endl;
}
TEST(ZobristTest, TranspositionIdentity) {
    // Target position: Four Knights Game after 3... Nf6

    // --- SETUP: Standard Move Order (A) ---
    Board board_A;
    board_A.read_fen(NotationInterface::starting_FEN());
    
    // 1. e4 e5 2. Nf3 Nc6 3. Nc3 Nf6
    std::vector<std::string> moves_A = {"e2e4", "e7e5", "g1f3", "b8c6", "b1c3", "g8f6"};
    Move m;
    for (const auto& uci : moves_A) {
        m = Move(uci);
        board_A.do_move(m);
    }
    uint64_t hash_A = ZobroistHasher::get().hash_board(board_A);

    // --- SETUP: Swapped Move Order (B) ---
    Board board_B;
    board_B.read_fen(NotationInterface::starting_FEN());

    // 1. e4 e5 2. Nc3 Nf6 3. Nf3 Nc6
    std::vector<std::string> moves_B = {"e2e4", "e7e5", "b1c3", "g8f6", "g1f3", "b8c6"};
    for (const auto& uci : moves_B){
        m = Move(uci);
        board_B.do_move(m);
    }
    uint64_t hash_B = ZobroistHasher::get().hash_board(board_B);

    // ASSERTION: The hashes MUST be the same
    ASSERT_EQ(hash_A, hash_B) << "Transposition failed: Identical positions have different hashes.";
    
    // Optional check: Ensure the final FENs are identical.
    // ASSERT_EQ(board_A.to_fen(), board_B.to_fen());
}
TEST(ZobristTest, MoveUndoIntegrity) {
    Board board;
    board.read_fen(NotationInterface::starting_FEN());
    
    uint64_t hash_A = ZobroistHasher::get().hash_board(board);

    std::string move_uci_string = "d2d4";
    
    // Create and perform the move
    Move move_object = Move(move_uci_string); 
    board.do_move(move_object); 
    uint64_t hash_B = ZobroistHasher::get().hash_board(board);
    
    ASSERT_NE(hash_A, hash_B) << "Hash did not change after move " << move_uci_string;

    // Undo the move
    board.undo_move(move_object);
    uint64_t hash_C = ZobroistHasher::get().hash_board(board);

    // ASSERTION 2: Hash must return to the original value
    ASSERT_EQ(hash_A, hash_C) << "Hash failed to revert after undo_move.";
}