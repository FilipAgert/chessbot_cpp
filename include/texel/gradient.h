#pragma once
#include "texel/eigen_defs.h"
#include <functional>
#include <vector>
namespace Texel {
class Gradient {
 public:
    virtual ~Gradient();
    virtual double evaluate(const dvec &params) const = 0;
    virtual dvec evaluate_grad(const dvec &params) const = 0;
    virtual std::tuple<double, dvec, dmat> eval_grad_hessian(const dvec &params) const = 0;
    mutable int num_evals = 0;
    mutable int num_grads = 0;
    mutable int num_h = 0;
};

/**
 * @brief Class for mixing gradient with integer and double parameters.
 *
 */
class MixedGradient : public Gradient {
 public:
    MixedGradient(std::vector<bool> is_index_double, std::function<double(const dvec &)> &f);
    inline double evaluate(const dvec &params) const override {
        num_evals++;
        return f(params);
    }

    dvec evaluate_grad(const dvec &params) const override;

    std::tuple<double, dvec, dmat> eval_grad_hessian(const dvec &params) const override;

 private:
    dvec stepsize;
    std::function<double(const dvec &)> f;
};
}  // namespace Texel