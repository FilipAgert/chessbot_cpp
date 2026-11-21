#pragma once

/**
 * @brief reads and stores parameters from json file
 *
 */

#include "eval/hce/parameters.h"
#include <string>
namespace Eval::HCE {
class ParameterConfig {
  public:
    EvalParameters params;

    explicit ParameterConfig(const std::string &filename);

    void load(const std::string &filename);
    void save(const std::string &filename) const;
};
} // namespace Eval::HCE