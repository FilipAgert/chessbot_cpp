#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace Texel {

struct TuningParameter {
    double lower = 0;
    double upper = 0;
    bool enabled = true;
    bool is_integer = true;
};

struct OptimizationParameter {
    std::string name;
    TuningParameter bounds;
    int count; // in the case of e.g. PST or other arrays.
};

class TuningConfig {
  private:
    nlohmann::json settings;

    std::unordered_map<std::string, TuningParameter> parameters;
    std::vector<OptimizationParameter> parameter_order;

    void add_parameter(const std::string &name, const nlohmann::json &j);

  public:
    explicit TuningConfig(const std::string &filename);

    bool enabled(const std::string &name) const;
    TuningParameter get(const std::string &name) const;
    bool contains(const std::string &name) const;

    const nlohmann::json &get_settings() const { return settings; }

    const std::vector<OptimizationParameter> &get_parameter_order() const { return parameter_order; }

    size_t size() const;
};

} // namespace Texel
