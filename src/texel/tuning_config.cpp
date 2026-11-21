#include "texel/tuning_config.h"
#include "core/piece.h"
#include "eval/hce/parameters.h"
#include "eval/hce/piece_square_table.h"
#include <fstream>
#include <stdexcept>

namespace Texel {

TuningConfig::TuningConfig(const std::string &filename) {
    std::ifstream file(filename);

    if (!file.is_open())
        throw std::runtime_error("Cannot open tuning config: " + filename);

    try {
        file >> settings;
    } catch (const nlohmann::json::exception &e) {
        throw std::runtime_error("Invalid tuning config: " + std::string(e.what()));
    }

    if (!settings.is_object())
        throw std::runtime_error("Tuning config must be a JSON object");

    // Scalars, piecevals and movevals
    for (const std::string group : {"scalars", "piecevals", "movevals"}) {
        if (!settings.contains(group))
            continue;

        for (const auto &[name, value] : settings.at(group).items()) {
            add_parameter(group + "." + name, value);
        }
    }

    // Passed pawn distances
    if (settings.contains("passed_dist_to_promotion")) {
        const auto &distances = settings.at("passed_dist_to_promotion");

        for (size_t i = 0; i < distances.size(); ++i) {
            add_parameter("passed_dist_to_promotion." + std::to_string(i + 1), distances[i]);
        }
    }

    // Piece square tables
    // Preserve the complete settings in `settings`.
    if (settings.contains("piece_square_tables")) {
        for (const auto &[name, value] : settings.at("piece_square_tables").items()) {
            add_parameter("piece_square_tables." + name, value);
        }
    }

    auto add_to_order = [&](const std::string &name, size_t count = 1) {
        if (contains(name) && enabled(name)) {
            parameter_order.push_back({name, get(name), count});
        }
    };

    // 1. Scalars
#define ADD_SCALAR(name) add_to_order("scalars." #name);
    EVAL_SCALAR_PARAMS(ADD_SCALAR)
#undef ADD_SCALAR

    // 2. Passed pawn distances
    for (size_t i = 1; i <= pieces::pawn; ++i) {
        add_to_order("passed_dist_to_promotion." + std::to_string(i));
    }

    // 3. Piece values
    for (size_t i = pieces::king; i <= pieces::pawn; ++i) {
        add_to_order("piecevals." + Piece::str_from_piece_type(i));
    }
    for (size_t i = pieces::king; i <= pieces::pawn; ++i) {
        add_to_order("movevals." + Piece::str_from_piece_type(i));
    }

    // 5. Piece square tables: piece order, early game then end game
    if (settings.contains("piece_square_tables")) {
        const auto &psts = settings.at("piece_square_tables");

        for (size_t i = pieces::king; i <= pieces::pawn; ++i) {
            for (const std::string stage : {"early_game", "end_game"}) {

                for (const auto &[name, pst] : psts.items()) {
                    Piece_t piece = Piece::piece_type_from_char(pst.at("piece").get<std::string>().at(0));

                    if (piece != i || pst.at("stage") != stage)
                        continue;

                    std::string key = "piece_square_tables." + name;

                    if (!contains(key) || !enabled(key))
                        continue;

                    Eval::HCE::Symmetry symmetry = Eval::HCE::symmetry_from_name(pst.value("symmetry", "NONE"));

                    size_t count = Eval::HCE::PieceSquareTable::num_params(symmetry, piece);

                    add_to_order(key, count);
                }
            }
        }
    }
}

void TuningConfig::add_parameter(const std::string &name, const nlohmann::json &j) {
    TuningParameter parameter{};

    // Enabled by default
    parameter.enabled = !j.value("disable", false);

    if (!j.contains("bounds")) {
        if (parameter.enabled)
            throw std::runtime_error("Missing bounds for tuning parameter: " + name);

        // Disabled parameters don't need bounds
        parameters[name] = parameter;
        return;
    }

    const auto &bounds = j.at("bounds");

    if (!bounds.is_array() || bounds.size() != 2 || !bounds[0].is_number_integer() || !bounds[1].is_number_integer()) {
        throw std::runtime_error("Invalid bounds for parameter: " + name);
    }

    parameter.lower = bounds[0].get<int>();
    parameter.upper = bounds[1].get<int>();

    if (parameter.lower > parameter.upper)
        throw std::runtime_error("Lower bound exceeds upper bound: " + name);

    parameters[name] = parameter;
}

bool TuningConfig::contains(const std::string &name) const { return parameters.contains(name); }

bool TuningConfig::enabled(const std::string &name) const {
    auto it = parameters.find(name);

    return it != parameters.end() && it->second.enabled;
}

TuningParameter TuningConfig::get(const std::string &name) const {
    auto it = parameters.find(name);

    if (it == parameters.end())
        throw std::runtime_error("Unknown tuning parameter: " + name);

    return it->second;
}

} // namespace Texel
