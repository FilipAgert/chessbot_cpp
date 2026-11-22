#pragma once
#include "Eigen/Core"
#include "eval/hce/hceval.h"
#include "parameters.h"
#include "texel/eigen_defs.h"
#include <Eigen/Sparse>
#include <pagmo/types.hpp>
#include <string>

namespace Texel {
using SparseDataSet = Eigen::SparseMatrix<double, Eigen::RowMajor>;

SparseDataSet build_dataset(std::vector<std::string> &games);

void evaluate_dataset(SparseDataSet &data);

} // namespace Texel