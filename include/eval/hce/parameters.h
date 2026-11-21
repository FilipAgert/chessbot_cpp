#pragma once
#include "eval/hce/piece_square_table.h"
#include <array>
#include <cassert>
#include <optional>

namespace Eval::HCE {
// contains all parameters needed to evaluate board

#define EVAL_SCALAR_PARAMS(X)                                                                                                                                  \
    X(bishop_double_bonus)                                                                                                                                     \
    X(king_dist)                                                                                                                                               \
    X(doubled_pawn_punishment)                                                                                                                                 \
    X(solo_pawn_punishment)                                                                                                                                    \
    X(king_pawn_proximity_score)                                                                                                                               \
    X(king_attacked_square_score)

class ParameterConfig;

struct EvalParameters {
    friend class ParameterConfig;

  private:
    EvalParameters() = default;

  public:
    EvalParameters(const EvalParameters &) = default;
    EvalParameters &operator=(const EvalParameters &) = default;
#define X(name) int name;
    EVAL_SCALAR_PARAMS(X)
#undef X
    static const int num_pieces = 7;
    std::array<int, num_pieces> piecevals{};
    std::array<int, num_pieces> movevals{};
    std::array<PieceSquareTable, num_pieces> psts;
    std::array<std::optional<PieceSquareTable>, num_pieces> psts_endgame;
    std::array<int, 7> passed_dist_to_promotion{};

    void set_pst(Piece_t p, Symmetry s, std::vector<int> params) {
        assert(p > 0 && p < num_pieces);
        psts[p].set(s, p, std::move(params));
    }
    void set_pst_endgame(Piece_t p, Symmetry s, std::vector<int> params) {
        assert(p > 0 && p < num_pieces);
        PieceSquareTable pst(s, p, params);
        psts_endgame[p] = std::make_optional(pst);
    }
    template <bool is_white> int pst(Piece_t p, int sq) const {
        assert(p > 0 && p < num_pieces);
        return psts.at(p).value<is_white>(sq);
    }

    std::vector<int> to_vector() const;
    void from_vector(const std::vector<int> &v);

    size_t num_params() const;
    std::string to_string() const;
};
} // namespace Eval::HCE