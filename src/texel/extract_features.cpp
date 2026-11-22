#include "texel/extract_features.h"
#include "parameters.h"
#include <iomanip>
namespace Texel {
SparseDataSet build_dataset(std::vector<std::string> &games) {
    Eval::HCE::FeatureAccumulator acc;
    int num_rows = games.size();
    int num_cols = acc.features.size();
    std::vector<Eigen::Triplet<double>> triplets;
    triplets.reserve(games.size() * 45);

    for (int i = 0; i < games.size(); i++) {
        acc.features.setZero();
        Board board(games[i]);
        Eval::HCE::HCEval::acc_features(board, acc);

        for (int col = 0; col < acc.features.size(); col++) {
            if (acc.features[col] != 0.0) {
                triplets.push_back({i, col, acc.features[col]});
            }
        }
    }
    SparseDataSet X(num_rows, num_cols);
    X.setFromTriplets(triplets.begin(), triplets.end());
    X.makeCompressed();
    return X;
}
void evaluate_dataset(SparseDataSet &data) {
    Eval::HCE::ParameterVector feature_weights = Eval::HCE::ParameterVector::Zero();

    for (int k = 0; k < data.outerSize(); ++k) {
        for (SparseDataSet::InnerIterator it(data, k); it; ++it) {
            feature_weights(it.col()) += std::abs(it.value());
        }
    }
    std::vector<Eigen::Index> indices(feature_weights.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&feature_weights](Eigen::Index a, Eigen::Index b) { return feature_weights(a) > feature_weights(b); });

    double avg_feature = feature_weights.mean();
    double median_feature = feature_weights[indices[indices.size() / 2]];

    std::cout << "Average feature weight: " << avg_feature << "\n";
    std::cout << "Median feature weight: " << median_feature << "\n";

    auto print_feature = [&](int rank) {
        Eigen::Index idx = indices[rank];

        std::cout << std::right << std::setw(4) << rank << ". " << std::left << std::setw(55) << Eval::HCE::FeatureLayout::feature_string(idx)
                  << " | Weight: " << std::right << std::setw(12) << std::fixed << std::setprecision(1) << feature_weights(idx) << '\n';
    };

    std::cout << "--- Largest 100 features and their weights ---\n";

    for (int i = static_cast<int>(indices.size()) - 1; i >= std::max(0, static_cast<int>(indices.size()) - 100); --i) {
        print_feature(i);
    }

    std::cout << "--- Smallest 100 features and their weights ---\n";

    for (int i = 100; i >= 0; --i) {
        print_feature(i);
    }
}
} // namespace Texel