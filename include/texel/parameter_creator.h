#pragma once

#include "eval/hce/parameters.h"
#include "texel/parameter_mapping.h"
#include "texel/tuning_config.h"
#include <pagmo/types.hpp>

namespace Texel {

/**
 * @brief Class responsible for taking the local parameter vector and mapping it back to the global one.
 * Minimizer sees perturbations starting at 0 from the initial parameters. bounds are mapped.
 *
 */
using Eval::HCE::ParameterVector;
class ParameterCreator {
  private:
    Eval::HCE::ParameterVector theta0; // initial parameters
    ParameterMapping mapping;
    Eigen::VectorXd lower_bound;
    Eigen::VectorXd upper_bound;

  public:
    int get_nlocal() const;

    ParameterCreator(Eval::HCE::EvalParameters &initial_parameters, const TuningConfig &config);

    ParameterVector create_global_params(const Eigen::VectorXd &local_perturbations) const;

    Eigen::VectorXd create_local_gradient(const ParameterVector &global_grad) const;

    const Eigen::VectorXd &get_lower_bound() const;
    const Eigen::VectorXd &get_upper_bound() const;
};

} // namespace Texel
