#include "texel/parameter_creator.h"
#include "Eigen/Core"
#include "parameters.h"
#include <stdexcept>

namespace Texel {
ParameterCreator::ParameterCreator(Eval::HCE::EvalParameters &initial_parameters, const TuningConfig &config)
    : theta0(initial_parameters.to_vector()), mapping(config) {
    using Eval::HCE::FeatureLayout;

    const int n_global = FeatureLayout::TOTAL_FEATURES;
    const int n_local = mapping.global_groups().size();

    // Absolute bounds for every global parameter.
    Eigen::VectorXd global_lower = Eigen::VectorXd::Constant(n_global, -std::numeric_limits<double>::infinity());

    Eigen::VectorXd global_upper = Eigen::VectorXd::Constant(n_global, std::numeric_limits<double>::infinity());

    // Populate global bounds in config order.
    // This assumes config order matches the local column order of A.
    int local_idx = 0;
    std::vector<std::vector<int>> groups = mapping.global_groups();

    for (const auto &param : config.get_parameter_order()) {
        if (!param.bounds.enabled)
            continue;

        // Find the global features belonging to this local parameter.
        for (size_t i = 0; i < param.count; i++) {
            const auto &group = groups[local_idx];

            for (int global_idx : group) {
                global_lower(global_idx) = param.bounds.lower;
                global_upper(global_idx) = param.bounds.upper;
            }

            ++local_idx;
        }
    }

    if (local_idx != n_local) {
        throw std::runtime_error("Mismatch between config parameters and mapping dimensions.");
    }

    // Initial absolute value of each local parameter.
    Eigen::VectorXd local_initial = mapping.to_local_avg(theta0);

    lower_bound.resize(n_local);
    upper_bound.resize(n_local);

    for (int i = 0; i < n_local; ++i) {
        double lower = -std::numeric_limits<double>::infinity();
        double upper = std::numeric_limits<double>::infinity();

        // Intersect bounds of all global features tied to this local parameter.
        for (int global_idx : groups[i]) {
            lower = std::max(lower, global_lower(global_idx));
            upper = std::min(upper, global_upper(global_idx));
        }

        if (lower > upper) {
            throw std::runtime_error("Incompatible bounds for local parameter " + std::to_string(i));
        }

        const double initial = local_initial(i);
        const double delta_lower = lower - initial;
        const double delta_upper = upper - initial;

        // Print local parameter and its associated global features.
        // std::cout << "\nLocal parameter " << i << "\n  Initial: " << initial << "\n  Absolute bounds: [" << lower << ", " << upper << "]"
        //           << "\n  Delta bounds:    [" << delta_lower << ", " << delta_upper << "]" << "\n  Global features:\n";

        // for (int global_idx : groups[i]) {
        //     std::cout << "    [" << global_idx << "] " << FeatureLayout::feature_string(global_idx) << " | initial: " << theta0(global_idx) << " | bounds: ["
        //               << global_lower(global_idx) << ", " << global_upper(global_idx) << "]\n";
        // }

        lower_bound(i) = delta_lower;
        upper_bound(i) = delta_upper;

        if (delta_lower > 0.0 || delta_upper < 0.0) {
            throw std::runtime_error("Initial parameter outside bounds at local index " + std::to_string(i));
        }
    }
}
ParameterVector ParameterCreator::create_global_params(const Eigen::VectorXd &local_perturbations) const {
    return theta0 + mapping.to_global(local_perturbations);
}
int ParameterCreator::get_nlocal() const { return lower_bound.size(); }
Eigen::VectorXd ParameterCreator::create_local_gradient(const ParameterVector &global_grad) const { return mapping.to_local_gradient(global_grad); }
const Eigen::VectorXd &ParameterCreator::get_lower_bound() const { return lower_bound; }
const Eigen::VectorXd &ParameterCreator::get_upper_bound() const { return upper_bound; }

} // namespace Texel