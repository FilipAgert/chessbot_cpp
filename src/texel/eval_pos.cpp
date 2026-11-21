#include "texel/eval_pos.h"
#include "board.h"
#include "eval/eval.h"
#include "eval/hce/hceval.h"
#include "eval/hce/parameters.h"
#include "integer_representation.h"
#include <omp.h>

namespace Texel {

double position_loss(const Eval::HCE::EvalParameters &params, const std::vector<std::string> &fens, float K, const dvec &res) {
    dvec pred = eval_positions(params, fens, K);
    return loss(res, pred);
}
dvec eval_positions(const Eval::HCE::EvalParameters &params, const std::vector<std::string> &fens, float K) {
    size_t num_pos = fens.size();
    dvec win_chance(num_pos);
    Eval::HCE::HCEval evaler(params);
#pragma omp parallel for
    for (size_t i = 0; i < num_pos; i++) {
        Board b(fens[i]);
        double sign = b.get_turn_color() == pieces::white ? 1.0 : -1.0;
        double eval = evaler.evaluate(b);
        double chance = sigmoid(sign * eval, K);
        win_chance(i) = chance;
    }
    return win_chance;
}

double loss(const dvec &res, const dvec &pred) { return std::sqrt((res - pred).array().square().sum() / res.size()); }
} // namespace Texel