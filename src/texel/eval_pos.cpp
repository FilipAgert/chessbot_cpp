#include "texel/eval_pos.h"
#include "board.h"
#include "eval.h"
#include "integer_representation.h"
#include <omp.h>

namespace Texel {
dvec eval_positions(const EvalParameters &params, const std::vector<std::string> &fens, float K) {
    size_t num_pos = fens.size();
    dvec win_chance(num_pos);
    EvalState evaler(params);
#pragma omp parallel for
    for (size_t i = 0; i < num_pos; i++) {
        Board b(fens[i]);
        double sign = b.get_turn_color() == pieces::white ? 1.0 : -1.0;
        double eval = evaler.eval(b);
        double chance = sigmoid(sign * eval, K);
        win_chance(i) = chance;
    }
    return win_chance;
}

double loss(const dvec &res, const dvec &pred) { return (res - pred).array().square().sum() / res.size(); }
}  // namespace Texel