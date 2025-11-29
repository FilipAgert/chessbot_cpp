// Copyright 2025 Filip Agert
#ifndef MOVEGEN_BENCHMARK_H
#define MOVEGEN_BENCHMARK_H
#include <board_state.h>
#include <string>
/**
 * @brief Class to assist with testing of movement generation. Timing movegeneration and for
 * checking number of moves from a state.
 */
class movegen_benchmark {
 public:
    /**
     * @brief Function to generate and get number of moves at a certain depth.
     *
     * @param[in] FEN Input FEN to start from
     * @param[in] depth Depth counts for half ply. Depth one is once, depth two is for both players
     * @return Number of moves at this depth.
     */
    int gen_num_moves(std::string FEN, int depth);

 private:
    int recurse_moves(BoardState state, int curr_depth, int to_depth);
};
#endif
