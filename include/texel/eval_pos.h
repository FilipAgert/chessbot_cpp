#pragma once
#include "eval/hce/parameters.h"
#include "texel/core.h"
#include "texel/eigen_defs.h"
#include <cmath>
#include <vector>
namespace Texel {


constexpr double BASE_K = 1.0;
inline double sigmoid(int eval, double K = BASE_K) {
    constexpr double log10 = 2.3;
    constexpr double mul = log10 / 400.0;
    return 1.0 / (1.0 + std::exp(-K * eval * mul));
}
inline double to_win(Result res) { return (static_cast<int>(res) + 1) * 0.5; }
} // namespace Texel