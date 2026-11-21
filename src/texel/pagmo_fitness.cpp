#include "texel/pagmo_fitness.h"
#include "eval/hce/parameters.h"
#include "texel/eval_pos.h"
#include <pagmo/problem.hpp>
#include <type_traits>

namespace Texel {
pagmo::vector_double PagmoLossWrapper::fitness(const pagmo::vector_double &x) const {
    Eval::HCE::EvalParameters params = p0;
    std::vector<int> p(x.size());

    for (size_t i = 0; i < x.size(); i++) {
        p.at(i) = std::round(x.at(i));
    }
    params.from_vector(p);
    return {position_loss(params, fens, K, res)};
}
} // namespace Texel