#include "texel/utils.h"
namespace Texel::Utils {
ParameterName parse_phase_name(const std::string &name) {
    std::vector<std::string> tok;
    size_t start = 0, pos;
    while ((pos = name.find('.', start)) != std::string::npos) {
        tok.push_back(name.substr(start, pos - start));
        start = pos + 1;
    }
    tok.push_back(name.substr(start));

    Phase phase = Phase::Shared;
    std::string base;
    for (const auto &t : tok) {
        Phase p = Phase::Shared;
        if (t == "early_game")
            p = Phase::MG;
        else if (t == "end_game")
            p = Phase::EG; // rename to EndGame if you like
        if (p != Phase::Shared) {
            if (phase != Phase::Shared)
                throw std::runtime_error("Two phases in name: " + name);
            phase = p;
            continue; // drop the phase token from the base
        }
        if (!base.empty())
            base += '.';
        base += t;
    }
    return {base, phase};
}
} // namespace Texel::Utils