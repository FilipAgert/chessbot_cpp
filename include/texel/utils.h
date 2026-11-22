#include "eval/hce/parameters.h"
namespace Texel::Utils {
using Eval::HCE::Phase;
struct ParameterName {
    std::string base;
    Phase phase;
};
ParameterName parse_phase_name(const std::string &name);
} // namespace Texel::Utils