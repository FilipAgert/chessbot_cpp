#pragma once
#include "eval/hce/parameters.h"
#include "parameters.h"
#include "texel/eigen_defs.h"
#include "texel/tuning_config.h"
#include "tuning_config.h"
#include <Eigen/Sparse>
namespace Texel {

using Eval::HCE::ParameterVector;
class ParameterMapping {
  private:
    Eigen::SparseMatrix<double, Eigen::ColMajor> A;  // local -> global mapping
    Eigen::SparseMatrix<double, Eigen::RowMajor> BT; // global->local AVERAGE. DO not use for to_local_gradient.

  public:
    ParameterMapping(const Texel::TuningConfig &config);
    ParameterVector to_global(const Eigen::VectorXd &local) const;

    Eigen::VectorXd to_local_gradient(const ParameterVector &global_gradient) const;

    /**
     * @brief Averages over global indices into local.
     *
     * @param global
     * @return Eigen::VectorXd
     */
    Eigen::VectorXd to_local_avg(const ParameterVector &global) const;

    std::vector<std::vector<int>> global_groups() const;
};
} // namespace Texel