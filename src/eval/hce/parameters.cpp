#include "eval/hce/parameters.h"
#include "notation_interface.h"
#include <sstream>
#include <stdexcept>
namespace Eval::HCE {
FeatureRef FeatureLayout::index_dispatch(bool white, Phase phase, Feature feature, Piece_t piece, int sq) {
    auto dispatch_piece = [&]<bool W, Phase P>() -> FeatureRef {
        switch (piece) {
        case pieces::pawn:
            return index<W, P, Feature::PST, pieces::pawn>(sq);
        case pieces::knight:
            return index<W, P, Feature::PST, pieces::knight>(sq);
        case pieces::bishop:
            return index<W, P, Feature::PST, pieces::bishop>(sq);
        case pieces::rook:
            return index<W, P, Feature::PST, pieces::rook>(sq);
        case pieces::queen:
            return index<W, P, Feature::PST, pieces::queen>(sq);
        case pieces::king:
            return index<W, P, Feature::PST, pieces::king>(sq);
        default:
            throw std::invalid_argument("Invalid piece");
        }
    };

    auto dispatch_piece_nosq = [&]<bool W, Phase P, Feature F>() -> FeatureRef {
        switch (piece) {
        case pieces::pawn:
            return index<W, P, F, pieces::pawn>();
        case pieces::knight:
            return index<W, P, F, pieces::knight>();
        case pieces::bishop:
            return index<W, P, F, pieces::bishop>();
        case pieces::rook:
            return index<W, P, F, pieces::rook>();
        case pieces::queen:
            return index<W, P, F, pieces::queen>();
        case pieces::king:
            return index<W, P, F, pieces::king>();
        default:
            throw std::invalid_argument("Invalid piece");
        }
    };

    auto dispatch_phase = [&]<bool W, Feature F>() -> FeatureRef {
        switch (phase) {
        case Phase::MG:
            if constexpr (F != Feature::Pieceval) {
                return dispatch_piece_nosq.template operator()<W, Phase::MG, F>();
            }
            break;
        case Phase::EG:
            if constexpr (F != Feature::Pieceval) {
                return dispatch_piece_nosq.template operator()<W, Phase::EG, F>();
            }
            break;
        case Phase::Shared:
            if constexpr (F == Feature::Pieceval) {
                return dispatch_piece_nosq.template operator()<W, Phase::Shared, F>();
            } else {
                throw std::invalid_argument("Invalid feature in shared phase");
            }
        }
        throw std::invalid_argument("Invalid phase");
    };

    auto dispatch_color = [&]<Feature F>() -> FeatureRef {
        return white ? dispatch_phase.template operator()<true, F>() : dispatch_phase.template operator()<false, F>();
    };

    switch (feature) {
    case Feature::PST:
        return white ? (phase == Phase::MG ? dispatch_piece.template operator()<true, Phase::MG>() : dispatch_piece.template operator()<true, Phase::EG>())
                     : (phase == Phase::MG ? dispatch_piece.template operator()<false, Phase::MG>() : dispatch_piece.template operator()<false, Phase::EG>());

    case Feature::PassedDistToPromo:
        return white ? (phase == Phase::MG ? index<true, Phase::MG, Feature::PassedDistToPromo, pieces::pawn>(sq)
                                           : index<true, Phase::EG, Feature::PassedDistToPromo, pieces::pawn>(sq))
                     : (phase == Phase::MG ? index<false, Phase::MG, Feature::PassedDistToPromo, pieces::pawn>(sq)
                                           : index<false, Phase::EG, Feature::PassedDistToPromo, pieces::pawn>(sq));

    case Feature::Moveval:
        return dispatch_color.template operator()<Feature::Moveval>();

    case Feature::Pieceval:
        return dispatch_color.template operator()<Feature::Pieceval>();
#define X(name)                                                                                                                                                \
    case Feature::name:                                                                                                                                        \
        return white ? (phase == Phase::MG ? index<true, Phase::MG, Feature::name>() : index<true, Phase::EG, Feature::name>())                                \
                     : (phase == Phase::MG ? index<false, Phase::MG, Feature::name>() : index<false, Phase::EG, Feature::name>());

        EVAL_SCALAR_PARAMS(X)
#undef X

    default:
        // Scalar features need individual compile-time feature cases.
        throw std::invalid_argument("Unsupported feature");
    }
}
std::string FeatureLayout::feature_string(int idx) {

    if (idx < 0 || idx >= TOTAL_FEATURES)
        throw std::invalid_argument("Index wrong. Does not exist");

    std::ostringstream oss;

    // Handle shared parameters separately.
    if (idx < SHARED_PARAMS) {
        Piece_t p = static_cast<Piece_t>(idx + 1);

        oss << "piecevalue (shared): " << Piece::str_from_piece_type(p);

        return oss.str();
    }

    // Determine phase and phase-relative index.
    std::string phase_string;
    int rel_idx;

    if (idx < END_GAME_OFFSET) {
        phase_string = "middle game";
        rel_idx = idx - MIDDLE_GAME_OFFSET;
    } else {
        phase_string = "end game";
        rel_idx = idx - END_GAME_OFFSET;
    }

    // extract which feature it is.
    // scalar
    // move
    // passed pawn
    // pst
    Feature f;
    int feature_rel_idx = 0;
    if (rel_idx < SCALAR_PARAMS) {
        int ctr = 0;
        if (false) {
        }
#define X(name)                                                                                                                                                \
    else if (rel_idx < ++ctr) {                                                                                                                                \
        f = Feature::name;                                                                                                                                     \
        feature_rel_idx = ctr - 1;                                                                                                                             \
    }
        EVAL_SCALAR_PARAMS(X)
#undef X
        else {
            throw std::runtime_error("Error in scalar param reading");
        }
    }

    else if (rel_idx < SCALAR_PARAMS + NUM_MOVEVALS) {
        feature_rel_idx = rel_idx - SCALAR_PARAMS;
        f = Feature::Moveval;
    } else if (rel_idx < SCALAR_PARAMS + NUM_MOVEVALS + PASSED_TO_PROMO) {
        feature_rel_idx = rel_idx - PASSED_OFFSET;
        f = Feature::PassedDistToPromo;
    } else if (rel_idx < SCALAR_PARAMS + NUM_MOVEVALS + PASSED_TO_PROMO + PST_TOT_PARAMS) {
        f = Feature::PST;
        feature_rel_idx = rel_idx - PST_KING_OFFSET;
    } else
        throw std::runtime_error("Error in reading feature");

    Piece_t p = pieces::none;
    int pst_idx = 0;
    if (f == Feature::Moveval || f == Feature::Pieceval) {
        p = feature_rel_idx + 1;
    } else if (f == Feature::PST) {
        p = (feature_rel_idx / PST_NON_PAWN_PARAMS) + 1;
        pst_idx = feature_rel_idx - (p - 1) * PST_NON_PAWN_PARAMS;
    }

    if (f == Feature::Pieceval || f == Feature::Moveval) {
        std::string feature_str = f == Feature::Pieceval ? "piecevalue" : "movevalue";
        oss << feature_str << " (" << phase_string << "): " << Piece::str_from_piece_type(p);
    } else if (f == Feature::PassedDistToPromo) {
        int dist = feature_rel_idx + 1;
        oss << "distance to promotion (" << phase_string << "): " << dist;
    } else if (f == Feature::PST) {
        int sq = pst_idx + (p == pieces::pawn ? 8 : 0);
        oss << "piece square table " << Piece::str_from_piece_type(p) << "(" << phase_string << "): " << NotationInterface::string_from_idx(sq);
    } else {
        std::string feature_str;
        if (false) {
        }
#define X(name) else if (f == Feature::name) feature_str = #name;
        EVAL_SCALAR_PARAMS(X)
#undef X
        else throw std::runtime_error("Invalid feature.");
        oss << feature_str << " (" << phase_string << ")";
    }
    return oss.str();

} // namespace Eval::HCE
std::string EvalParameters::to_string() const {
    std::ostringstream os;

    auto dump_scalars = [&](const auto &params, const char *label) {
        os << "\n[" << label << "]\n";
#define X(name) os << #name << " = " << params.name << "\n";
        EVAL_SCALAR_PARAMS(X)
#undef X
    };

    auto dump_array = [&]<size_t N>(const char *label, const std::array<int, N> &a) {
        os << label << " = {";
        for (size_t p = 1; p < N; ++p)
            os << a[p] << (p + 1 < N ? ", " : "");
        os << "}\n";
    };

    auto dump_psts = [&](const auto &psts, const char *label) {
        static const char *names[num_pieces] = {"", "King", "Queen", "Rook", "Knight", "Bishop", "Pawn"};

        os << "\n[" << label << " PSTs]\n";

        for (size_t p = 1; p < num_pieces; ++p) {
            os << "\n" << names[p] << " PST\n";
            os << psts[p].to_string() << "\n";
        }
    };

    // Shared parameters
    os << "[Shared]\n";
    dump_array("piecevals[K,Q,R,N,B,P]", shared.piecevals);

    // Middlegame parameters
    dump_scalars(mg, "Middlegame");
    dump_array("movevals[K,Q,R,N,B,P]", mg.movevals);
    dump_array("dist2promo[1,2,3,4,5,6]", mg.passed_dist_to_promotion);
    dump_psts(mg.psts, "Middlegame");

    // Endgame parameters
    dump_scalars(eg, "Endgame");
    dump_array("movevals[K,Q,R,N,B,P]", eg.movevals);
    dump_array("dist2promo[1,2,3,4,5,6]", eg.passed_dist_to_promotion);
    dump_psts(eg.psts, "Endgame");

    return os.str();
}

ParameterVector EvalParameters::to_vector() {
    ParameterVector params{};

    int shared_count = 0;
    for_each_piece([&](auto p_const) {
        constexpr Piece_t p = p_const.value;
        constexpr int index = FeatureLayout::index<true, Phase::Shared, Feature::Pieceval, p>().index;
        params[index] = shared.piecevals[p];
        shared_count++;
    });
    assert(shared_count == FeatureLayout::SHARED_PARAMS);

    auto serialize_phase = [&]<Phase phase>(const auto &phase_params) {
        int cnt = 0;

#define X(name)                                                                                                                                                \
    params[FeatureLayout::index<true, phase, Feature::name>().index] = phase_params.name;                                                                      \
    cnt++;
        EVAL_SCALAR_PARAMS(X)
#undef X

        for_each_piece([&](auto p_const) {
            constexpr Piece_t p = p_const.value;
            constexpr int index = FeatureLayout::index<true, phase, Feature::Moveval, p>().index;
            params[index] = phase_params.movevals[p];
            cnt++;
        });

        for (uint8_t row = 1; row <= 6; row++) {
            uint8_t sq = NotationInterface::idx(row, 0);
            int index = FeatureLayout::index<true, phase, Feature::PassedDistToPromo, pieces::pawn>(sq).index;
            params[index] = phase_params.passed_dist_to_promotion[7 - row];
            cnt++;
        }

        for_each_piece([&](auto p_const) {
            constexpr Piece_t p = p_const.value;
            int lb = p == pieces::pawn ? 8 : 0;
            int ub = p == pieces::pawn ? 56 : 64;
            const PieceSquareTable &pst = phase_params.psts[p];
            const std::array<int, 64> &white = pst.get<true>();

            for (int sq = lb; sq < ub; sq++, cnt++) {
                int index = FeatureLayout::index<true, phase, Feature::PST, p>(sq).index;
                params[index] = white[sq];
            }
        });
        return cnt;
    };
    int mg_count = serialize_phase.template operator()<Phase::MG>(mg);
    assert(mg_count == FeatureLayout::MIDDLE_GAME_PARAMS);

    int eg_count = serialize_phase.template operator()<Phase::EG>(eg);
    assert(eg_count == FeatureLayout::END_GAME_PARAMS);
    assert(eg_count + mg_count + shared_count == FeatureLayout::TOTAL_FEATURES);
    return params;
}
EvalParameters::EvalParameters(const ParameterVector &params) {
    // Shared piece values
    int shared_count = 0;

    for_each_piece([&](auto p_const) {
        constexpr Piece_t p = p_const.value;
        constexpr int index = FeatureLayout::index<true, Phase::Shared, Feature::Pieceval, p>().index;

        shared.piecevals[p] = params[index];
        shared_count++;
    });

    assert(shared_count == FeatureLayout::SHARED_PARAMS);

    // Deserialize MG and EG parameters
    auto deserialize_phase = [&]<Phase phase>(auto &phase_params) {
        int cnt = 0;

#define X(name)                                                                                                                                                \
    phase_params.name = params[FeatureLayout::index<true, phase, Feature::name>().index];                                                                      \
    cnt++;

        EVAL_SCALAR_PARAMS(X)
#undef X

        // Move values
        for_each_piece([&](auto p_const) {
            constexpr Piece_t p = p_const.value;
            constexpr int index = FeatureLayout::index<true, phase, Feature::Moveval, p>().index;

            phase_params.movevals[p] = params[index];
            cnt++;
        });

        // Passed pawn distance to promotion
        for (uint8_t row = 1; row <= 6; row++) {
            uint8_t sq = NotationInterface::idx(row, 0);

            int index = FeatureLayout::index<true, phase, Feature::PassedDistToPromo, pieces::pawn>(sq).index;

            phase_params.passed_dist_to_promotion[7 - row] = params[index];

            cnt++;
        }

        // Piece-square tables
        for_each_piece([&](auto p_const) {
            constexpr Piece_t p = p_const.value;

            int lb = p == pieces::pawn ? 8 : 0;
            int ub = p == pieces::pawn ? 56 : 64;

            std::vector<int> param_pst(p == pieces::pawn ? 48 : 64);

            for (int sq = lb, pst_idx = 0; sq < ub; sq++, cnt++, pst_idx++) {
                int index = FeatureLayout::index<true, phase, Feature::PST, p>(sq).index;

                param_pst[pst_idx] = params[index];
            }
            // Adapt this line to your PieceSquareTable constructor.
            phase_params.psts[p] = PieceSquareTable(Symmetry::None, p, param_pst);
        });

        return cnt;
    };

    int mg_count = deserialize_phase.template operator()<Phase::MG>(mg);

    assert(mg_count == FeatureLayout::MIDDLE_GAME_PARAMS);

    int eg_count = deserialize_phase.template operator()<Phase::EG>(eg);

    assert(eg_count == FeatureLayout::END_GAME_PARAMS);

    assert(shared_count + mg_count + eg_count == FeatureLayout::TOTAL_FEATURES);
}
} // namespace Eval::HCE