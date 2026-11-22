#pragma once
#include "core/notation_interface.h"
#include "eval/hce/piece_square_table.h"
#include "texel/eigen_defs.h"
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

enum class Feature : uint8_t {
#define X(name) name,
    EVAL_SCALAR_PARAMS(X)
#undef X
        Pieceval,
    Moveval,
    PassedDistToPromo,
    PST
};

enum class Phase : uint8_t { Shared, MG, EG };
struct FeatureRef {
    int index;
    int sign = 1;
};
struct FeatureLayout {
    // Layout:
    //  Shared:
    //  - piece values [king - pawn]
    //
    //  Middle game:
    //  Scalar parameters
    //  Move values
    //  passed pawn values
    //  piece square tables
    //  king ... pawn
    //
    //  end game: duplicated as mid game
    static constexpr int NUM_PIECES = 6; // we count king and none here. for faster extraction later.

    static constexpr int PST_NON_PAWN_PARAMS = 64;
    static constexpr int PST_PAWN_PARAMS = 48;
    static constexpr int PST_TOT_PARAMS = PST_PAWN_PARAMS + 5 * PST_NON_PAWN_PARAMS;
#define X(name) +1
    static constexpr int SCALAR_PARAMS = EVAL_SCALAR_PARAMS(X);
#undef X
    static constexpr int SHARED_PARAMS = NUM_PIECES;

    static constexpr int NUM_MOVEVALS = NUM_PIECES;
    static constexpr int PASSED_TO_PROMO = 6;
    static constexpr int MIDDLE_GAME_PARAMS = SCALAR_PARAMS + NUM_MOVEVALS + PASSED_TO_PROMO + PST_TOT_PARAMS;
    static constexpr int END_GAME_PARAMS = MIDDLE_GAME_PARAMS;

    static constexpr int TOTAL_FEATURES = SHARED_PARAMS + MIDDLE_GAME_PARAMS + END_GAME_PARAMS;

    static constexpr int PIECEVAL_OFFSET = 0;
    static constexpr int MIDDLE_GAME_OFFSET = SHARED_PARAMS;
    static constexpr int END_GAME_OFFSET = MIDDLE_GAME_OFFSET + MIDDLE_GAME_PARAMS;
    // side offsets: from 0. then combined with middlegame/ endgame offsets
    static constexpr int SCALAR_OFFSET = 0;
    static constexpr int MOVEVAL_OFFSET = SCALAR_OFFSET + SCALAR_PARAMS;
    static constexpr int PASSED_OFFSET = MOVEVAL_OFFSET + NUM_MOVEVALS;

    // pst offsets
    static constexpr int PST_KING_OFFSET = PASSED_OFFSET + PASSED_TO_PROMO;
    static constexpr int PST_PAWN_OFFSET = PST_KING_OFFSET + PST_NON_PAWN_PARAMS * 5;

    static_assert(TOTAL_FEATURES == END_GAME_OFFSET + PST_PAWN_OFFSET + PST_PAWN_PARAMS);

    template <Piece_t p> constexpr static int piece_index() {
        static_assert(p > pieces::none && p <= pieces::pawn);
        return static_cast<int>(p) - 1;
    }
    template <Piece_t p> constexpr static int pst_offset() {
        const int pidx = piece_index<p>();
        return pidx * PST_NON_PAWN_PARAMS; // its okay since pawn is at the end
    }
    static FeatureRef index_dispatch(bool white, Phase phase, Feature feature, Piece_t piece = pieces::none, int sq = -1);

    // scalar features:
    template <bool is_white, Phase phase, Feature feature> static constexpr FeatureRef index() {
        const int scalar_idx = static_cast<int>(feature);
        static_assert(phase != Phase::Shared);

        const int phase_offset = phase == Phase::MG ? MIDDLE_GAME_OFFSET : phase == Phase::EG ? END_GAME_OFFSET : 0;

        return {phase_offset + SCALAR_OFFSET + scalar_idx, is_white ? 1 : -1};
    }

    // for movevals or piecevals
    template <bool is_white, Phase phase, Feature feature, Piece_t piece> static constexpr FeatureRef index() {
        const int pidx = piece_index<piece>();
        static_assert(feature == Feature::Pieceval || feature == Feature::Moveval);
        static_assert((feature == Feature::Pieceval && phase == Phase::Shared) || (feature == Feature::Moveval && phase != Phase::Shared));
        const int offset = feature == Feature::Pieceval ? PIECEVAL_OFFSET
                           : phase == Phase::MG         ? MIDDLE_GAME_OFFSET + MOVEVAL_OFFSET
                                                        : END_GAME_OFFSET + MOVEVAL_OFFSET;

        return {offset + pidx, is_white ? 1 : -1};
    }

    template <bool is_white, Phase phase, Feature feature, Piece_t piece> static constexpr FeatureRef index(int sq) {
        if constexpr (feature == Feature::PassedDistToPromo && piece == pieces::pawn) {
            // passed pawn feature.
            int row = NotationInterface::row(sq);
            int dist_to_promo = is_white ? 7 - row : row;
            static_assert(feature == Feature::PassedDistToPromo);
            static_assert(phase != Phase::Shared);
            const int offset = PASSED_OFFSET + (phase == Phase::EG ? END_GAME_OFFSET : MIDDLE_GAME_OFFSET);
            return {dist_to_promo - 1 + offset, is_white ? 1 : -1};

        } else {
            // peace square table.
            static_assert(feature == Feature::PST);
            static_assert(phase != Phase::Shared);
            // start of PST?
            const int offset = PST_KING_OFFSET + pst_offset<piece>() + ((phase == Phase::EG) ? END_GAME_OFFSET : MIDDLE_GAME_OFFSET);
            int sq_w = is_white ? sq : sq ^ 56;
            if constexpr (piece == pieces::pawn) {
#ifndef NDEBUG
                int row = NotationInterface::row(sq);
                assert(row != 0 && row != 7);
#endif
                // we can shift down by 8. Row amount.
                sq_w -= 8;
            }
            return {offset + sq_w, is_white ? 1 : -1};
        }
    }

    /**
     * @brief Short descriptor of the feature you are indexing.
     *
     * @param idx
     * @return std::string
     */
    static std::string feature_string(int idx);
};
template <typename Func> constexpr void for_each_piece(Func &&f) {
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        // pieces::king is 1 .. pieces::pawn is 6
        (f(std::integral_constant<Piece_t, static_cast<Piece_t>(Is + 1)>{}), ...);
    }(std::make_index_sequence<FeatureLayout::NUM_PIECES>{});
}
using ParameterVector = Eigen::Matrix<double, FeatureLayout::TOTAL_FEATURES, 1>;

struct EvalParameters {
    friend class ParameterConfig;

  private:
    EvalParameters() = default;

  public:
    EvalParameters(const EvalParameters &) = default;
    EvalParameters &operator=(const EvalParameters &) = default;
    EvalParameters(const ParameterVector &v);
    static const int num_pieces = 7;

    struct {
        std::array<int, num_pieces> piecevals{};
    } shared;
    struct {
#define X(name) int name;
        EVAL_SCALAR_PARAMS(X)
#undef X
        std::array<int, num_pieces> movevals{};
        std::array<int, 7> passed_dist_to_promotion{};
        std::array<PieceSquareTable, num_pieces> psts;

        template <Feature feature> int get() const {
            if constexpr (false) {
            }
#define X(name) else if constexpr (feature == Feature::name) return name;
            EVAL_SCALAR_PARAMS(X)
#undef X
            else {
                static_assert(feature != feature, "Unhandled feature value");
            }
        }
    } mg;
    struct {
#define X(name) int name;
        EVAL_SCALAR_PARAMS(X)
#undef X
        std::array<int, num_pieces> movevals{};
        std::array<int, 7> passed_dist_to_promotion{};
        std::array<PieceSquareTable, num_pieces> psts;
        template <Feature feature> int get() const {
            if constexpr (false) {
            }
#define X(name) else if constexpr (feature == Feature::name) return name;
            EVAL_SCALAR_PARAMS(X)
#undef X
            else {
                static_assert(feature != feature, "Unhandled feature value");
            }
        }
    } eg;
    std::string to_string() const;

    /**
     * @brief Create the parameter vector corresponding to the eval.
     *
     * @return ParameterVector
     */
    ParameterVector to_vector();
};

} // namespace Eval::HCE