#pragma once

#include "eval/hce/parameters.h"
#include <nlohmann/json.hpp>
#include <string>

namespace Eval::HCE {

class ParameterConfig {
  public:
    EvalParameters params;

    explicit ParameterConfig(const std::string &filename);

    void load(const std::string &filename);

    void save(const std::string &filename, const nlohmann::json &stats = nlohmann::json::object()) const;
};

} // namespace Eval::HCE