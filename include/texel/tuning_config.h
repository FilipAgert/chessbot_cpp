#pragma once
#include <string>
#include <unordered_map>
namespace Texel {
struct ParameterBounds {
    double lower;
    double upper;
};

class TuningConfig {
  public:
    std::unordered_map<std::string, ParameterBounds> bounds;

    explicit TuningConfig(const std::string &filename);

    bool is_tuned(const std::string &name) const;
    ParameterBounds get_bounds(const std::string &name) const;
};
} // namespace Texel