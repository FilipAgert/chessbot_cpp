#pragma once

#include "eval/eval.h"
#include "eval/hce/parameters.h"
#include "pagmo/types.hpp"
#include "parameters.h"
#include "texel/eigen_defs.h"
#include <pagmo/algorithm.hpp>
#include <pagmo/problem.hpp>
#include <vector>

namespace Texel {
class PagmoLossWrapper {
  public:
    const std::vector<std::string> fens;
    double K;
    const dvec res;
    const Eval::HCE::EvalParameters p0;
    pagmo::vector_double fitness(const pagmo::vector_double &) const;

    std::pair<pagmo::vector_double, pagmo::vector_double> get_bounds() const {
        std::vector<int> initial = p0.to_vector();

        size_t num = initial.size();

        pagmo::vector_double lb(num), ub(num);

        for (size_t i = 0; i < num; i++) {
            double range = 20 + 0.1 * initial[i];

            lb[i] = std::round(initial[i] - range);
            ub[i] = std::round(initial[i] + range);
        }

        return {lb, ub};
    }
    pagmo::vector_double::size_type get_nobj() const { return 1; }

    pagmo::vector_double::size_type get_nix() const { return p0.num_params(); }
};
static_assert(std::is_copy_constructible_v<Texel::PagmoLossWrapper>, "Wrapper is not copy constructible");

static_assert(pagmo::is_udp<Texel::PagmoLossWrapper>::value, "Wrapper does not satisfy PaGMO UDP requirements");
} // namespace Texel