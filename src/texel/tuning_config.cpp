#include "texel/tuning_config.h"
#include "core/piece.h"
#include "eval/hce/parameters.h"
#include "eval/hce/piece_square_table.h"

#include <fstream>
#include <stdexcept>
#include <unordered_set>
namespace Texel {

namespace {

constexpr const char *EARLY_GAME = "early_game";
constexpr const char *END_GAME = "end_game";

// Add a shared parameter, or separate early-game/end-game parameters.
template <typename AddParameter> void add_phase_parameter(const std::string &name, const nlohmann::json &value, AddParameter add) {
    const bool has_eg = value.is_object() && value.contains(EARLY_GAME);
    const bool has_lg = value.is_object() && value.contains(END_GAME);

    if (has_eg || has_lg) {
        if (!(has_eg && has_lg))
            throw std::runtime_error(name + ": split parameter must define both early_game and end_game "
                                            "(use \"disable\": true to freeze one phase)");
        if (value.contains("bounds"))
            throw std::runtime_error(name + ": cannot have both shared 'bounds' and phase entries");
        add(name + "." + EARLY_GAME, value.at(EARLY_GAME));
        add(name + "." + END_GAME, value.at(END_GAME));
    } else {
        add(name, value);
    }
}

} // namespace

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

    auto add = [&](const std::string &name, const nlohmann::json &value) { add_parameter(name, value); };

    // 1. Scalars: shared values or separate phase values.
    if (settings.contains("scalars")) {
        const auto &scalars = settings.at("scalars");

        if (!scalars.is_object())
            throw std::runtime_error("'scalars' must be a JSON object");

        for (const auto &[name, value] : scalars.items()) {
            add_phase_parameter("scalars." + name, value, add);
        }
    }

    // 2. Piece values are shared between phases.
    if (settings.contains("piecevals")) {
        const auto &piecevals = settings.at("piecevals");

        if (!piecevals.is_object())
            throw std::runtime_error("'piecevals' must be a JSON object");

        for (const auto &[name, value] : piecevals.items()) {
            add("piecevals." + name, value);
        }
    }

    // 3. Mobility values: shared or phase-specific.
    if (settings.contains("movevals")) {
        const auto &movevals = settings.at("movevals");

        if (!movevals.is_object())
            throw std::runtime_error("'movevals' must be a JSON object");

        for (const auto &[name, value] : movevals.items()) {
            add_phase_parameter("movevals." + name, value, add);
        }
    }

    // 4. Passed pawn distances.
    if (settings.contains("passed_dist_to_promotion")) {
        const auto &distances = settings.at("passed_dist_to_promotion");
        const int NUM_RANKS = 6;
        auto add_distances = [&](const std::string &stage, const nlohmann::json &array) {
            if (!array.is_array() || array.size() != NUM_RANKS)
                throw std::runtime_error("passed_dist_to_promotion" + (stage.empty() ? "" : "." + stage) + " must be an array of " + std::to_string(NUM_RANKS) +
                                         " entries");

            for (size_t i = 0; i < array.size(); ++i) {
                std::string name = "passed_dist_to_promotion.";

                if (!stage.empty())
                    name += stage + ".";

                name += std::to_string(i + 1);
                add(name, array[i]);
            }
        };

        const bool has_eg = distances.is_object() && distances.contains(EARLY_GAME);
        const bool has_lg = distances.is_object() && distances.contains(END_GAME);
        if (has_eg || has_lg) {
            if (!(has_eg && has_lg))
                throw std::runtime_error("passed_dist_to_promotion: need both early_game and end_game arrays");
            add_distances(EARLY_GAME, distances.at(EARLY_GAME));
            add_distances(END_GAME, distances.at(END_GAME));
        } else {
            add_distances("", distances);
        }
    }

    // 5. Piece-square tables.
    if (settings.contains("piece_square_tables")) {
        const auto &psts = settings.at("piece_square_tables");

        if (!psts.is_object())
            throw std::runtime_error("'piece_square_tables' must be a JSON object");

        for (const auto &[name, value] : psts.items()) {
            add("piece_square_tables." + name, value);
        }
    }
    std::unordered_set<std::string> consumed;

    auto add_to_order = [&](const std::string &name, size_t count = 1) {
        if (!contains(name))
            return;
        consumed.insert(name);
        if (enabled(name))
            parameter_order.push_back({name, get(name), static_cast<int>(count)});
    };

    auto add_phase_to_order = [&](const std::string &name) {
        if (contains(name)) {
            add_to_order(name);
        } else {
            add_to_order(name + "." + EARLY_GAME);
            add_to_order(name + "." + END_GAME);
        }
    };

    // 1. Scalars.
#define ADD_SCALAR(name) add_phase_to_order("scalars." #name);
    EVAL_SCALAR_PARAMS(ADD_SCALAR)
#undef ADD_SCALAR

    // 2. Passed pawn distances.
    for (size_t i = 1; i <= pieces::pawn; ++i) {
        const std::string base = "passed_dist_to_promotion." + std::to_string(i);

        if (contains(base)) {
            add_to_order(base);
        } else {
            add_to_order("passed_dist_to_promotion." + std::string(EARLY_GAME) + "." + std::to_string(i));

            add_to_order("passed_dist_to_promotion." + std::string(END_GAME) + "." + std::to_string(i));
        }
    }

    // 3. Piece values.
    for (size_t i = pieces::king; i <= pieces::pawn; ++i) {
        add_to_order("piecevals." + Piece::str_from_piece_type(i));
    }

    // 4. Mobility values.
    for (size_t i = pieces::king; i <= pieces::pawn; ++i) {
        add_phase_to_order("movevals." + Piece::str_from_piece_type(i));
    }

    // 5. Piece-square tables: piece order, early game then end game.
    if (settings.contains("piece_square_tables")) {
        const auto &psts = settings.at("piece_square_tables");

        for (size_t i = pieces::king; i <= pieces::pawn; ++i) {
            for (const std::string stage : {EARLY_GAME, END_GAME}) {
                for (const auto &[name, pst] : psts.items()) {
                    Piece_t piece = Piece::piece_type_from_char(pst.at("piece").get<std::string>().at(0));

                    if (piece != i || pst.at("stage") != stage)
                        continue;

                    const std::string key = "piece_square_tables." + name;

                    if (!contains(key) || !enabled(key))
                        continue;

                    const auto symmetry = Eval::HCE::symmetry_from_name(pst.value("symmetry", "NONE"));

                    const size_t count = Eval::HCE::PieceSquareTable::num_params(symmetry, piece);
                    consumed.insert(key);

                    add_to_order(key, count);
                }
            }
        }
    }
    for (const auto &[name, p] : parameters)
        if (!consumed.contains(name))
            throw std::runtime_error("Tuning parameter '" + name + "' is not recognised (typo, or name/stage mismatch)");
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
