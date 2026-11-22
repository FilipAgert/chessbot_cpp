#pragma once
#include "core/board.h"
#include "core/piece.h"
#include "eval/eval.h"
#include "eval/hce/parameters.h"
#include "eval/hce/piece_square_table.h"
#include "eval/hce/utils.h"

namespace Eval::HCE {
struct EvalScore {
    int mg = 0;
    int eg = 0;
};

using FeatureVector = Eigen::Matrix<double, FeatureLayout::TOTAL_FEATURES, 1>;

struct ScoreAccumulator {
    int shared = 0;
    int mg = 0;
    int eg = 0;
    const EvalParameters &params;
    template <bool is_white, Feature feat, Piece_t p = pieces::none> void add(int val, int sq = -1) {
        constexpr int sign = is_white ? 1 : -1;
        if constexpr (feat == Feature::Pieceval) {
            shared += sign * val * params.shared.piecevals[p];
        } else if constexpr (feat == Feature::Moveval) {
            mg += sign * val * params.mg.movevals[p];
            eg += sign * val * params.eg.movevals[p];
        } else if constexpr (feat == Feature::PST) {
            mg += sign * val * params.mg.psts[p].template value<is_white>(sq);
            eg += sign * val * params.eg.psts[p].template value<is_white>(sq);
        } else if constexpr (feat == Feature::PassedDistToPromo) {
            int row = NotationInterface::row(sq);
            int dist = is_white ? 7 - row : row;
            mg += sign * val * params.mg.passed_dist_to_promotion[dist];
            eg += sign * val * params.eg.passed_dist_to_promotion[dist];
        } else {
            // Scalar parameters (doubled_pawn_punishment, bishop_double_bonus, etc.)
            mg += sign * val * params.mg.get<feat>();
            eg += sign * val * params.eg.get<feat>();
        }
    }
};
struct FeatureAccumulator {
    FeatureVector features = FeatureVector::Zero();
    int phase = 0;
    template <bool is_white, Feature feat, Piece_t p = pieces::none> void add(double val, int sq = -1) {
        constexpr int sign = is_white ? 1 : -1;
        if constexpr (feat == Feature::Pieceval) {
            features[FeatureLayout::index<is_white, Phase::Shared, feat, p>().index] += sign * val;
        } else if constexpr (feat == Feature::Moveval) {
            features[FeatureLayout::index<is_white, Phase::MG, feat, p>().index] += Utils::eval_phase(phase, sign * val, false);
            features[FeatureLayout::index<is_white, Phase::EG, feat, p>().index] += Utils::eval_phase(phase, sign * val, true);
        } else if constexpr (feat == Feature::PST) {
            features[FeatureLayout::index<is_white, Phase::MG, feat, p>(sq).index] += Utils::eval_phase(phase, sign * val, false);
            features[FeatureLayout::index<is_white, Phase::EG, feat, p>(sq).index] += Utils::eval_phase(phase, sign * val, true);
        } else if constexpr (feat == Feature::PassedDistToPromo) {
            features[FeatureLayout::index<is_white, Phase::MG, feat, p>(sq).index] += Utils::eval_phase(phase, sign * val, false);
            features[FeatureLayout::index<is_white, Phase::EG, feat, p>(sq).index] += Utils::eval_phase(phase, sign * val, true);
        } else {
            features[FeatureLayout::index<is_white, Phase::MG, feat>().index] += Utils::eval_phase(phase, sign * val, false);
            features[FeatureLayout::index<is_white, Phase::EG, feat>().index] += Utils::eval_phase(phase, sign * val, true);
        }
    }
};

class HCEval : public Eval {
  public:
    HCEval(EvalParameters params);

    int evaluate(const Board &board) override;

    template <typename Acc> static void acc_features(const Board &board, Acc &acc);
    void push_move(Move move, const Board &board) override;
    void pop_move(Move move, const Board &board) override;
    bool delta_futility_pruning(int standing_pat, int alpha, int threshhold, const Board &board) override;

    const HCE::EvalParameters &get_params() const override;

  private:
    static constexpr std::array<int, 7> piecevals_basic = {0, 200000, 900, 500, 300, 300, 100};
    const EvalParameters params;
    template <bool is_white> bool delta_futility_pruning_(int standing_pat, int alpha, int threshhold, const Board &board);
    template <typename Acc, Piece_t p = 1> static void eval_pst(const Board &board, Acc &acc);
    template <typename Acc, Piece_t p> static void eval_single_pst(const Board &board, Acc &acc);
    template <typename Acc, Piece_t p = 1> static int eval_material_diff(const Board &board, Acc &acc);

    template <typename Acc> static void eval_king_safety(const Board &board, Acc &acc);
    template <typename Acc> static void eval_mobility(const Board &board, Acc &acc);
    /**
     * @brief Evaluates king distance to centre score. At early game its negative, encouraging
     * safe play. Late game its positive, encouraging using the king.
     *
     * @param[in] state board state
     * @return evaluation of king distance to centre
     */
    template <typename Acc> static void eval_king_distance(const Board &board, int mat_diff, Acc &acc);

    /**
     * @brief Score for pawn structure. Passed pawns, unprotected pawns, blocking pawns, etc.
     *
     * @param[in] board board to check.
     * @return score for pawn structure, normalised by white winning is positive.
     */
    template <typename Acc> static void eval_pawn_structure(const Board &board, Acc &acc);
};
} // namespace Eval::HCE