#include "texel/texel_problem.h"
#include "pagmo/types.hpp"

namespace Texel {
Evaluation TexelProblem::evaluate(const pagmo::vector_double &x) const {
    using Eval::HCE::ParameterVector;
    using dvec = Eigen::VectorXd;
    Eigen::Map<const Eigen::VectorXd> local(x.data(), x.size());

    ParameterVector global = creator->create_global_params(local);

    dvec scores = data->operator*(global);

    constexpr double ln10 = 2.3;
    constexpr double mul = ln10 / 400.0;
    dvec preds = 1.0 / (1.0 + ((-K * mul) * scores.array()).exp());
    dvec diff = preds - targets;
    double loss = std::sqrt(diff.array().square().sum() / data->rows());
    return {scores, preds, diff, loss};
}
pagmo::vector_double TexelProblem::fitness(const pagmo::vector_double &x) const { return {evaluate(x).loss}; }

pagmo::vector_double TexelProblem::gradient(const pagmo::vector_double &x) const {
    Evaluation eval = evaluate(x);

    constexpr double ln10 = 2.3;
    constexpr double mul = ln10 / 400.0;
    const int N = data->rows();
    dvec delta = mul * K * eval.diff.array() * eval.predictions.array() * (1.0 - eval.predictions.array());
    ParameterVector grad = data->transpose() * delta / (N * eval.loss);

    Eigen::VectorXd local_grad = creator->create_local_gradient(grad);

    // convert to pagmo.
    return pagmo::vector_double(local_grad.begin(), local_grad.begin() + local_grad.size());
}

std::pair<pagmo::vector_double, pagmo::vector_double> TexelProblem::get_bounds() const {
    using dvec = Eigen::VectorXd;
    const dvec &lower = creator->get_lower_bound();
    const dvec &upper = creator->get_upper_bound();
    return std::make_pair(pagmo::vector_double(lower.begin(), lower.begin() + lower.size()), pagmo::vector_double(upper.begin(), upper.begin() + upper.size()));
}
pagmo::vector_double TexelProblem::get_initial_guess() const { return pagmo::vector_double(data->rows(), 0.0); }
} // namespace Texel