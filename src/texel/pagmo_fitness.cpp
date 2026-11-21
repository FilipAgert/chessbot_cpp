#include "texel/pagmo_fitness.h"
#include "eval/hce/parameters.h"
#include "texel/eval_pos.h"
#include <pagmo/problem.hpp>
#include <type_traits>

namespace Texel {
pagmo::vector_double PagmoLossWrapper::fitness(const pagmo::vector_double &x) const {
    Eval::HCE::EvalParameters params = param_creator->create_params(x);
    return {position_loss(params, fens, K, res)};
}
} // namespace Texel