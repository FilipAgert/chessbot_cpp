// Copyright 2025 Filip Agert
#ifndef MOVEGEN_BENCHMARK_H
#define MOVEGEN_BENCHMARK_H
#include <board.h>
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
     * @param[in] print_depth How deep to print number of moves found in each branch. -1 for no
     * printing. Default = -1.
     * @return Number of moves at this depth.
     */
    static int gen_num_moves(std::string FEN, int depth, int print_depth = -1);
    static int gen_num_moves(Board board, int depth, int print_depth = -1);

 private:
    template <bool is_white> static int recurse_moves(Board board, int print_depth, int curr_depth, int to_depth);
    static std::array<std::array<Move, max_legal_moves>, 32> move_arr;
};
#endif
