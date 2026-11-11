#pragma once
#include "eval.h"
#include "texel/core.h"
#include "texel/eigen_defs.h"
#include <cmath>
#include <vector>
namespace Texel {

dvec eval_positions(const EvalParameters &params, const std::vector<std::string> &fens, float K);

constexpr double BASE_K = 1.0;
inline double sigmoid(int eval, double K = BASE_K) {
    constexpr double log10 = 2.3;
    constexpr double mul = log10 / 400.0;
    return 1.0 / (1.0 + std::exp(-K * eval * mul));
}
inline double to_win(Result res) { return (static_cast<int>(res) + 1) * 0.5; }
/**
 * @brief
 *
 * @param res results in 0: black win 1/2: draw, 1: white win
 * @param pred
 * @return float
 */
double loss(const dvec &res, const dvec &pred);
}  // namespace Texel