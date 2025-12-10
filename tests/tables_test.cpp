
#include <board.h>
#include <gtest/gtest.h>
#include <notation_interface.h>
#include <tables.h>

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

TEST(TranspositionTest, default_invalid) {
    Board state;
    state.read_fen(NotationInterface::starting_FEN());
    transposition_table tab;
    uint64_t hash = ZobroistHasher::get().hash_board(state);
    ASSERT_FALSE(tab.get(hash));
    tab.store(hash, Move{2, 9}, 0, transposition_entry::exact, 0);
    ASSERT_TRUE(tab.get(hash));
}
