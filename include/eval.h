// Copyright 2025 Filip Agert
#ifndef EVAL_H
#define EVAL_H
#include "board_state.h"
class EvalState {
 public:
    static int eval(BoardState &state);
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

 private:
    static int eval_material(std::vector<Piece> pieces);
    static int eval_mobility(BoardState &state);
};

struct PieceValue {
    static constexpr int king = 200000;
    static constexpr int pawn = 100;
    static constexpr int knight = 290;
    static constexpr int bishop = 300;
    static constexpr int rook = 500;
    static constexpr int queen = 900;
    static constexpr std::array<int, 7> piecevals = {0, king, queen, rook, knight, bishop, pawn};

    static constexpr int inv_frac =
        10;  // Fraction of extra value piece is worth extra from having more spaces to move to.
    // Formula is : piece_val * num_moves / max_possible_moves * frac
    static constexpr int pawn_moveval = 0;
    static constexpr int knight_moveval = knight / (8 * inv_frac);
    static constexpr int bishop_moveval = bishop / (14 * inv_frac);
    static constexpr int rook_moveval = rook / (14 * inv_frac);
    static constexpr int queen_moveval = 0;
    static constexpr int king_moveval = -6;
    static constexpr std::array<int, 7> movevals = {
        0, king_moveval, queen_moveval, rook_moveval, knight_moveval, bishop_moveval, pawn_moveval};
};

#endif
