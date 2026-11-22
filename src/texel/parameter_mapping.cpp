

#include "texel/parameter_mapping.h"
#include "Eigen/Core"
#include "parameters.h"
#include "piece_square_table.h"
#include "texel/utils.h"
#include <stdexcept>
namespace Texel {
using namespace Texel::Utils;

ParameterMapping::ParameterMapping(const TuningConfig &config) {
    std::vector<Eigen::Triplet<double>> triplets;
    // a triplet is a {row, col, value} config. used to create sparse matrix.
    //

    // rows is global
    // cols are local
    using Eval::HCE::Feature;
    using Eval::HCE::FeatureLayout;
    int local_idx = 0;
    const std::vector<OptimizationParameter> &params = config.get_parameter_order();
    for (int i = 0; i < params.size(); i++) {
        const OptimizationParameter &param = params[i];
        if (!param.bounds.enabled)
            continue;
        const std::string &name = param.name;
        const ParameterName parsed = parse_phase_name(name);

        // piece value!
        auto do_piecemove = [&](Feature f, Phase p) {
            std::string prefix = f == Feature::Pieceval ? "piecevals." : "movevals.";
            bool found_piece = false;
            if (parsed.base.find(prefix) == 0) {
                const std::string piece_name = parsed.base.substr(prefix.size());
                for (size_t i = pieces::king; i <= pieces::pawn; ++i) {
                    if (piece_name != Piece::str_from_piece_type(i))
                        continue;

                    Eigen::Triplet<double> trip(FeatureLayout::index_dispatch(true, p, f, i).index, local_idx, 1.0);
                    triplets.push_back(trip);
                    found_piece = true;
                }
                if (!found_piece)
                    throw std::runtime_error("Should not have entered this if.");
            }
            return found_piece;
        };
        if (do_piecemove(Feature::Pieceval, Phase::Shared)) {
            local_idx++;
            continue;
        }

        // if you get past here, we are only in parameters that have middle game and endgame variants.
        // BUT if we provide Phase shared, these should be spread across both global features.
        std::vector<Phase> phases;
        if (parsed.phase == Phase::Shared)
            phases = {Phase::MG, Phase::EG};
        else
            phases = {parsed.phase};

        bool used_local = false;
        for (Phase phase : phases) {
#define ASSIGN_SCALAR(name_)                                                                                                                                   \
    if (parsed.base.find("scalars." #name_) == 0) {                                                                                                            \
        Eigen::Triplet<double> trip(FeatureLayout::index_dispatch(true, phase, Feature::name_).index, local_idx, 1.0);                                         \
        triplets.push_back(trip);                                                                                                                              \
        used_local = true;                                                                                                                                     \
        continue;                                                                                                                                              \
    }
            // we have scalar parameter. add to vector
            EVAL_SCALAR_PARAMS(ASSIGN_SCALAR)
#undef ASSIGN_SCALAR

            // passed pawn
            if (parsed.base.find("passed_dist_to_promotion.") == 0) {
                const size_t row = std::stoul(parsed.base.substr(parsed.base.find_last_of('.') + 1));
                // index operators on square, not distance.
                // distance = row. its okay
                // but make sure to put color black for distance to equal the row.
                assert(row != 0 && row != 7);
                int sq = NotationInterface::idx(row, 0);
                int global = FeatureLayout::index_dispatch(false, phase, Feature::PassedDistToPromo, pieces::pawn, sq).index;
                Eigen::Triplet<double> trip(global, local_idx, 1.0);
                used_local = true;
                triplets.push_back(trip);
                continue;
            }

            // mobility values
            if (do_piecemove(Feature::Moveval, phase)) {
                used_local = true;
                continue;
            }
        }
        if (used_local) {
            local_idx++;
            continue;
        }
        // pst is special case...
        if (name.find("piece_square_tables.") == 0) {
            const std::string pst_name = name.substr(std::string("piece_square_tables.").size());

            const auto &pst = config.get_settings().at("piece_square_tables").at(pst_name);
            Piece_t piece = Piece::piece_type_from_char(pst.at("piece").get<std::string>().at(0));

            auto symmetry = Eval::HCE::symmetry_from_name(pst.at("symmetry"));

            const std::string stage = pst.at("stage").get<std::string>();
            std::vector<std::vector<int>> pst_groups = Eval::HCE::PieceSquareTable::get_parameter_groups(symmetry, piece);

            Phase pst_phase;
            if (stage == "early_game") {
                pst_phase = Phase::MG;
            } else if (stage == "end_game") {
                pst_phase = Phase::EG;
            } else {
                throw std::runtime_error("Invalid PST stage: " + stage);
            }

            if (static_cast<int>(pst_groups.size()) != param.count) {
                throw std::runtime_error("PST parameter count mismatch for " + name);
            }

            for (const std::vector<int> &squares : pst_groups) {
                for (int sq : squares) {
                    int global = FeatureLayout::index_dispatch(true, pst_phase, Feature::PST, piece, sq).index;

                    triplets.emplace_back(global, local_idx, 1.0);
                }

                ++local_idx;
            }
            continue;
            // check so stage is the same as phase??
        }

        throw std::runtime_error("Unknown optimization parameter: " + name);
    }
    // global features
    int num_rows = FeatureLayout::TOTAL_FEATURES;
    int num_cols = local_idx;
    Eigen::SparseMatrix<double> T(num_rows, num_cols);
    T.setFromTriplets(triplets.begin(), triplets.end());
    T.makeCompressed();

    A = std::move(T);
    BT = A.transpose();

    // 2. Normalize each row (local parameter) by the number of global features mapped to it
    for (int k = 0; k < BT.outerSize(); ++k) {
        double row_sum = 0.0;
        for (Eigen::SparseMatrix<double, Eigen::RowMajor>::InnerIterator it(BT, k); it; ++it) {
            row_sum += it.value();
        }
        if (row_sum > 0.0) {
            for (Eigen::SparseMatrix<double, Eigen::RowMajor>::InnerIterator it(BT, k); it; ++it) {
                it.valueRef() /= row_sum;
            }
        }
    }
} // namespace Texel
std::vector<std::vector<int>> ParameterMapping::global_groups() const {
    std::vector<std::vector<int>> groups(A.cols());

    for (int col = 0; col < A.outerSize(); ++col) {
        // A is column-major: each column represents one local parameter.
        for (Eigen::SparseMatrix<double>::InnerIterator it(A, col); it; ++it) {
            groups[col].push_back(it.row());
        }
    }

    return groups;
}
ParameterVector ParameterMapping::to_global(const Eigen::VectorXd &local) const { return A * local; }

Eigen::VectorXd ParameterMapping::to_local_gradient(const Eval::HCE::ParameterVector &global_gradient) const { return A.transpose() * global_gradient; }
Eigen::VectorXd ParameterMapping::to_local_avg(const Eval::HCE::ParameterVector &global) const { return BT * global; }
} // namespace Texel