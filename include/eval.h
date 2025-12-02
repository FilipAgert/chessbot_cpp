// Copyright 2025 Filip Agert
#ifndef EVAL_H
#define EVAL_H
#include "board_state.h"
class EvalState {
    static int eval(BoardState state);
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
