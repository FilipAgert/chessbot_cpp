#include "texel/gradient.h"
#include <cassert>
namespace Texel {
Gradient::~Gradient() = default;

dvec MixedGradient::evaluate_grad(const dvec &params) const {
    num_grads++;
    dvec grad(params.size());
    assert(params.size() == stepsize.size());
    dvec params_cpy(params);
    for (size_t i = 0; i < params.size(); i++) {
        double h = stepsize(i);
        double original = params(i);
        params_cpy(i) += h;
        double ep1 = evaluate(params_cpy);
        params_cpy(i) -= 2 * h;
        double em1 = evaluate(params_cpy);
        grad(i) = (ep1 - em1) / (2 * h);
        params_cpy(i) = original;
    }
    return grad;
}
std::tuple<double, dvec, dmat> MixedGradient::eval_grad_hessian(const dvec &params) const {
    num_h++;
    size_t np = params.size();

    dvec grad(np);
    dmat H(np, np);

    dvec fp(np), fm(np);
    dvec x(params);
    double f0 = evaluate(params);

    // prefill vectors
    for (size_t i = 0; i < np; i++) {
        double h = stepsize(i);
        x(i) += h;
        fp(i) = evaluate(x);
        x(i) -= 2.0 * h;
        fm(i) = evaluate(x);
        x(i) += h;
        grad(i) = (fp(i) - fm(i)) / (2 * h);
    }

    for (size_t i = 0; i < np; i++) {
        double hi = stepsize(i);
        for (size_t j = i; j < np; j++) {
            double hj = stepsize(j);
            x(i) += hi;
            x(j) += hj;
            double fpp = evaluate(x);
            x(i) -= 2 * hi;
            x(j) -= 2 * hj;
            double fmm = evaluate(x);
            x(i) += hi;
            x(j) += hj;

            H(i, j) = (fpp - fp(i) - fp(j) + 2 * f0 - fm(i) - fm(j) + fmm) / (2 * hi * hj);
            H(j, i) = H(i, j);
        }
    }
    return std::make_tuple(f0, grad, H);
}

MixedGradient::MixedGradient(std::vector<bool> is_index_double, std::function<double(const dvec &)> &f) : f(f) {
    stepsize.resize(is_index_double.size());
    double h = 1e-6;
    for (size_t i = 0; i < is_index_double.size(); i++) {
        if (is_index_double[i]) {
            stepsize(i) = h;
        } else {
            stepsize(i) = 1.0;
        }
    }
}
}  // namespace Texel