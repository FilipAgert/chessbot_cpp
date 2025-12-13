// Copyright 2025 Filip Agert
#include <board.h>
#include <constants.h>
#include <iostream>
#include <move.h>
#include <movegen_benchmark.h>
#include <string>
int movegen_benchmark::gen_num_moves(std::string FEN, int depth, int print_depth) {
    Board board;
    bool success = board.read_fen(FEN);
    if (!success) {
        std::cerr << "There was an error reading the FEN. Exiting." << std::endl;
        exit(EXIT_FAILURE);
    }
    return gen_num_moves(board, depth, print_depth);
}
int movegen_benchmark::gen_num_moves(Board state, int depth, int print_depth) {
    bool is_white = state.get_turn_color() == pieces::white;
    if (is_white)
        return recurse_moves<true>(state, print_depth, 0, depth);
    else
        return recurse_moves<false>(state, print_depth, 0, depth);
}

template <bool is_white>
int movegen_benchmark::recurse_moves(Board state, int print_depth, int curr_depth, int to_depth) {
    if (curr_depth == to_depth)
        return 1;

    std::array<Move, max_legal_moves> moves;
    int num_moves = state.get_moves<normal_search, is_white>(moves);
    int total_moves = 0;
    for (int i = 0; i < num_moves; i++) {
        state.do_move(moves[i]);
        int this_move_nbr = recurse_moves<!is_white>(state, print_depth, curr_depth + 1, to_depth);
        if (curr_depth <= print_depth) {
            for (int j = 0; j < curr_depth; j++)
                std::cout << "    ";  // indent by depth
            std::cout << moves[i].toString() << ": " << this_move_nbr << "\n";
        }
        total_moves += this_move_nbr;
        state.undo_move(moves[i]);
    }
    return total_moves;
}
