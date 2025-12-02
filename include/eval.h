// Copyright 2025 Filip Agert
#ifndef EVAL_H
#define EVAL_H
#include "board_state.h"
class EvalState {
 public:
    static int eval(BoardState state);
    /**
     * @brief Sorts the moves in array by the scores in scores
     *
     * @param[[TODO:direction]] moves [TODO:description]
     * @param[[TODO:direction]] scores [TODO:description]
     * @param[[TODO:direction]] num_moves [TODO:description]
     * @param[in] ascending: bool to sort in ascending order. If false sorts by descending order.
     * True if minimiser, false if maximiser.
     */
    static void partial_move_sort(std::array<Move, max_legal_moves> &moves,
                                  std::array<int, max_legal_moves> &scores, size_t num_moves,
                                  bool ascending);
};

struct PieceValue {
    static constexpr int pawn = 100;
    static constexpr int knight = 290;
    static constexpr int bishop = 300;
    static constexpr int rook = 500;
    static constexpr int queen = 800;
    static constexpr std::array<int, 7> piecevals = {0, 0, queen, rook, knight, bishop, pawn};
};

#endif
