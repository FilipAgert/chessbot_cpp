#pragma once

#include "eval/hce/parameters.h"
#include "texel/tuning_config.h"
#include <pagmo/types.hpp>

namespace Texel {

class HCEParameterCreator {
  private:
    Eval::HCE::EvalParameters p0;
    TuningConfig config;

  public:
    HCEParameterCreator(Eval::HCE::EvalParameters p0, TuningConfig config);
    Eval::HCE::EvalParameters create_params(const pagmo::vector_double &x) const;

    std::pair<pagmo::vector_double, pagmo::vector_double> get_bounds() const;

    pagmo::vector_double::size_type get_nobj() const { return 1; }

    pagmo::vector_double::size_type get_nix() const;

    pagmo::vector_double get_initial_guess() const;
};

} // namespace Texel
