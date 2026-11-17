#pragma once
#include "core/board.h"
#include "core/piece.h"
#include "eval/eval.h"
#include "eval/hce/parameters.h"
#include "eval/hce/piece_square_table.h"

namespace Eval::HCE {

class HCEval : public Eval {
  public:
    HCEval();
    HCEval(EvalParameters params);

    int evaluate(const Board &board) override;
    void push_move(Move move, const Board &board) override;
    void pop_move(Move move, const Board &board) override;
    bool delta_futility_pruning(int standing_pat, int alpha, int threshhold, const Board &board) override;

    const HCE::EvalParameters &get_params() const override;

  private:
    const EvalParameters params;
    template <bool is_white> bool delta_futility_pruning_(int standing_pat, int alpha, int threshhold, const Board &board);
    template <Piece_t p = 1> int eval_pst(const Board &board, int phase);
    template <Piece_t p> int eval_single_pst(const Board &board, const PieceSquareTable &pst);
    template <Piece_t p = 1> int eval_material_diff(const Board &board) const;

    int eval_king_safety(const Board &board, int phase) const;
    int eval_mobility(const Board &board);
    /**
     * @brief Evaluates king distance to centre score. At early game its negative, encouraging
     * safe play. Late game its positive, encouraging using the king.
     *
     * @param[in] state board state
     * @return evaluation of king distance to centre
     */
    int eval_king_distance(const Board &board, int mat_diff, int phase);

    /**
     * @brief Score for pawn structure. Passed pawns, unprotected pawns, blocking pawns, etc.
     *
     * @param[in] board board to check.
     * @return score for pawn structure, normalised by white winning is positive.
     */
    int eval_pawn_structure(const Board &board);
};
} // namespace Eval::HCE