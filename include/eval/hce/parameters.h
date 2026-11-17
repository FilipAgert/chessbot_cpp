#pragma once
#include "eval/hce/piece_square_table.h"
#include <array>
#include <cassert>
#include <optional>

namespace Eval::HCE {
// contains all parameters needed to evaluate board
namespace DefaultVals {
constexpr int king = 200000;
constexpr int pawn = 100;
constexpr int knight = 290;
constexpr int bishop = 300;
constexpr int rook = 500;
constexpr int queen = 900;

constexpr int inv_frac = 10; // Fraction of extra value piece is worth extra from having more spaces to move to.
constexpr int pawn_moveval = 0;
constexpr int knight_moveval = knight / (8 * inv_frac);
constexpr int bishop_moveval = bishop / (14 * inv_frac);
constexpr int rook_moveval = rook / (14 * inv_frac);
constexpr int queen_moveval = 0;
constexpr int king_moveval = -6;

constexpr int bishop_double_bonus = 25;
constexpr int king_dist =
    10; // This more or less corresponds to maximum value of king distance score in late game. (8cp/sq at full lategame with maximum material advantage.)
constexpr int doubled_pawn_punishment = -15;
constexpr int solo_pawn_punishment = -15;

// king safety
constexpr int king_pawn_proximity_score = 10;   // 10 per pawn in front of king.
constexpr int king_attacked_square_score = -15; // -15 per attacked square near the king.
} // namespace DefaultVals

#define EVAL_SCALAR_PARAMS(X)                                                                                                                                  \
    X(bishop_double_bonus, DefaultVals::bishop_double_bonus)                                                                                                   \
    X(king_dist, DefaultVals::king_dist)                                                                                                                       \
    X(doubled_pawn_punishment, DefaultVals::doubled_pawn_punishment)                                                                                           \
    X(solo_pawn_punishment, DefaultVals::solo_pawn_punishment)                                                                                                 \
    X(king_pawn_proximity_score, DefaultVals::king_pawn_proximity_score)                                                                                       \
    X(king_attacked_square_score, DefaultVals::king_attacked_square_score)

struct EvalParameters {
#define X(name, def) int name = def;
    EVAL_SCALAR_PARAMS(X)
#undef X
    static const int num_pieces = 7;
    std::array<int, num_pieces> piecevals{
        0, DefaultVals::king, DefaultVals::queen, DefaultVals::rook, DefaultVals::knight, DefaultVals::bishop, DefaultVals::pawn};
    std::array<int, num_pieces> movevals{0,
                                         DefaultVals::king_moveval,
                                         DefaultVals::queen_moveval,
                                         DefaultVals::rook_moveval,
                                         DefaultVals::knight_moveval,
                                         DefaultVals::bishop_moveval,
                                         DefaultVals::pawn_moveval};
    std::array<PieceSquareTable, num_pieces> psts;
    std::array<std::optional<PieceSquareTable>, num_pieces> psts_endgame;
    std::array<int, 7> passed_dist_to_promotion = {0, 80, 60, 40, 30, 10, 10};

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
    EvalParameters(std::array<std::pair<std::vector<int>, Symmetry>, num_pieces> psqs = psq_parameters_base,
                   std::array<std::optional<std::pair<std::vector<int>, Symmetry>>, 7> psqs_endgame = psq_parameters_base_endgame) {
        for (Piece_t p = 1; p < num_pieces; p++) {
            set_pst(p, psqs.at(p).second, psqs.at(p).first);
        }
        for (Piece_t p = 1; p < num_pieces; p++) {
            if (psqs_endgame[p]) {
                set_pst_endgame(p, psqs_endgame[p].value().second, psqs_endgame[p].value().first);
            }
        }
    }
};
} // namespace Eval::HCE