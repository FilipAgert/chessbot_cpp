// board_test.cpp
#include "constants.h"
#include <board_state.h>
#include <gtest/gtest.h>
#include <string>

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