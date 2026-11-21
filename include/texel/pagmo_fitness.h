#pragma once

#include "eval/hce/parameters.h"
#include "pagmo/types.hpp"
#include "texel/eigen_defs.h"
#include "texel/hce_create_params.h"
#include <pagmo/algorithm.hpp>
#include <pagmo/problem.hpp>
#include <vector>

namespace Texel {
class PagmoLossWrapper {
  private:
    std::shared_ptr<HCEParameterCreator> param_creator;

  public:
    std::vector<std::string> fens;
    double K;
    dvec res;

    PagmoLossWrapper() = default;

    PagmoLossWrapper(std::vector<std::string> fens, double K, dvec res, HCEParameterCreator param_creator)
        : param_creator(std::make_shared<HCEParameterCreator>(std::move(param_creator))), fens(std::move(fens)), K(K), res(std::move(res)) {}

    pagmo::vector_double fitness(const pagmo::vector_double &x) const;

    std::pair<pagmo::vector_double, pagmo::vector_double> get_bounds() const { return param_creator->get_bounds(); }

    pagmo::vector_double::size_type get_nobj() const { return 1; }

    pagmo::vector_double::size_type get_nix() const { return param_creator->get_nix(); }
};

static_assert(std::is_copy_constructible_v<Texel::HCEParameterCreator>, "Wrapper is not copy constructible");
static_assert(std::is_copy_constructible_v<Texel::PagmoLossWrapper>, "Wrapper is not copy constructible");

static_assert(pagmo::is_udp<Texel::PagmoLossWrapper>::value, "Wrapper does not satisfy PaGMO UDP requirements");
} // namespace Texel