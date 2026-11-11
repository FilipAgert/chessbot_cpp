#pragma once
#include "texel/gradient.h"
namespace Texel {
struct MinimizeResult {
    dvec x;
    double f;
    dvec grad;
    dvec H;
    constexpr static int success = 0;
    constexpr static int max_steps = 1;
    constexpr static int hessian_non_invertible = -1;
    constexpr static int non_descent = -2;
    int flag = 1;  // 0 minimum found 1 max steps reached, -1 hessian non-invertible
};
MinimizeResult minimize(const dvec &x0, const Gradient &grad, std::function<std::string(dvec)> print_fnc);
}  // namespace Texel