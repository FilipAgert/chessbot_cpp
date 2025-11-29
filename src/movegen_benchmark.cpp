// Copyright 2025 Filip Agert
#include <board_state.h>
#include <constants.h>
#include <iostream>
#include <move.h>
#include <movegen_benchmark.h>
#include <string>
int movegen_benchmark::gen_num_moves(std::string FEN, int depth) {
    BoardState boardState;
    bool success = boardState.read_fen(FEN);
    if (!success) {
        std::cerr << "There was an error reading the FEN. Exiting." << std::endl;
        exit(EXIT_FAILURE);
    }
    return recurse_moves(boardState, 0, depth);
}

int movegen_benchmark::recurse_moves(BoardState state, int curr_depth, int to_depth) {
    if (curr_depth == to_depth)
        return 1;

    std::array<Move, max_legal_moves> moves;
    int num_moves = state.get_moves(moves);
    int total_moves = 0;
    for (int i = 0; i < num_moves; i++) {
        state.do_move(moves[i]);
        total_moves += recurse_moves(state, curr_depth + 1, to_depth);
        state.undo_move(moves[i]);
    }
    return total_moves;
}
