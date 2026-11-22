#pragma once
#include "texel/extract_features.h"
#include "texel/parameter_creator.h"
#include <pagmo/problem.hpp>
#include <pagmo/types.hpp>
namespace Texel {
struct Evaluation {
    Eigen::VectorXd scores;
    Eigen::VectorXd predictions;
    Eigen::VectorXd diff;
    double loss;
};

class TexelProblem {
  private:
    std::shared_ptr<ParameterCreator> creator;
    std::shared_ptr<SparseDataSet> data;
    Eigen::VectorXd targets;
    double K;

    Evaluation evaluate(const pagmo::vector_double &x) const;

  public:
    TexelProblem(ParameterCreator creator_, std::shared_ptr<SparseDataSet> data, Eigen::VectorXd targets, double K)
        : creator(std::make_shared<ParameterCreator>(std::move(creator_))), data(data), targets(std::move(targets)), K(K) {}
    TexelProblem() = default;
    pagmo::vector_double fitness(const pagmo::vector_double &x) const;

    pagmo::vector_double gradient(const pagmo::vector_double &x) const;

    std::pair<pagmo::vector_double, pagmo::vector_double> get_bounds() const;

    pagmo::vector_double::size_type get_nobjs() const { return 1; }

    pagmo::vector_double::size_type get_nix() const { return 0; }

    pagmo::vector_double get_initial_guess() const;
};
static_assert(std::is_copy_constructible_v<TexelProblem>, "Wrapper is not copy constructible");

static_assert(pagmo::is_udp<Texel::TexelProblem>::value, "problem does not satisfy pagmo udp requirement");
} // namespace Texel