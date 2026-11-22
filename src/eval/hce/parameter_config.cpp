#include "eval/hce/parameter_config.h"
#include "eval/hce/parameters.h"
#include "piece_square_table.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace Eval::HCE {
using json = nlohmann::json;

ParameterConfig::ParameterConfig(const std::string &filename) { load(filename); }
template <typename T> void load_phase_value(const json &j, const std::string &key, T &mg, T &eg) {
    if (!j.contains(key))
        throw std::runtime_error("Parameter file does not contain key" + key);

    const auto &value = j.at(key);

    if (value.is_number()) {
        mg = value.get<T>();
        eg = value.get<T>();
        return;
    }

    if (value.is_object()) {
        if (value.contains("early_game"))
            mg = value.at("early_game").get<T>();
        else
            throw std::runtime_error("Does not contain early game but contains end game" + key);

        if (value.contains("end_game"))
            eg = value.at("end_game").get<T>();
        else
            throw std::runtime_error("Does not contain end_game but contains early game" + key);

        return;
    }

    throw std::runtime_error("Invalid phase parameter: " + key);
}
void ParameterConfig::load(const std::string &filename) {
    std::ifstream file(filename);
    json j = json::parse(file);
    EvalParameters p;

    auto piecevals = j.value("piecevals", json::object());
    p.shared.piecevals[pieces::king] = piecevals.value("king", 200000);
    p.shared.piecevals[pieces::queen] = piecevals.value("queen", 1009);
    p.shared.piecevals[pieces::rook] = piecevals.value("rook", 570);
    p.shared.piecevals[pieces::bishop] = piecevals.value("bishop", 350);
    p.shared.piecevals[pieces::knight] = piecevals.value("knight", 336);
    p.shared.piecevals[pieces::pawn] = piecevals.value("pawn", 130);

    auto movevals = j.value("movevals", json::object());

#define LOAD_MOVEVAL(name, piece) load_phase_value(movevals, name, p.mg.movevals[piece], p.eg.movevals[piece]);

    LOAD_MOVEVAL("king", pieces::king);
    LOAD_MOVEVAL("queen", pieces::queen);
    LOAD_MOVEVAL("rook", pieces::rook);
    LOAD_MOVEVAL("bishop", pieces::bishop);
    LOAD_MOVEVAL("knight", pieces::knight);
    LOAD_MOVEVAL("pawn", pieces::pawn);

#undef LOAD_MOVEVAL
    auto passed = j.value("passed_dist_to_promotion", json::object());

    auto load_distances = [](const json &values, auto &target) {
        for (size_t i = 0; i < 6; ++i)
            target[i + 1] = values.at(i).get<int>();
    };

    if (passed.is_array()) {
        load_distances(passed, p.mg.passed_dist_to_promotion);
        load_distances(passed, p.eg.passed_dist_to_promotion);
    } else if (passed.is_object()) {
        if (passed.contains("early_game"))
            load_distances(passed["early_game"], p.mg.passed_dist_to_promotion);

        if (passed.contains("end_game"))
            load_distances(passed["end_game"], p.eg.passed_dist_to_promotion);
    }

    auto scalars = j.value("scalars", json::object());

#define X(name) load_phase_value(scalars, #name, p.mg.name, p.eg.name);

    EVAL_SCALAR_PARAMS(X)

#undef X

    // piece square table loading
    auto pst = j.value("piece_square_tables", json::object());

    for (const auto &[piece_name, piece_data] : pst.items()) {
        Piece_t piece = Piece::piece_type_from_char(piece_data.at("piece").get<std::string>().at(0));
        Symmetry s = symmetry_from_name(piece_data.at("symmetry").get<std::string>());
        std::string stage = piece_data.at("stage").get<std::string>();
        int num_params = PieceSquareTable::num_params(s, piece);
        std::vector<int> empty(num_params, 0);
        std::vector<int> params = piece_data.value("values", empty);
        if (stage == "early_game")
            p.mg.psts[piece] = PieceSquareTable(s, piece, params);
        else if (stage == "end_game")
            p.eg.psts[piece] = PieceSquareTable(s, piece, params);
        else
            std::cerr << "error parsing stage: " << stage << std::endl;
    }
    params = p;
}
void ParameterConfig::save(const std::string &filename, const json &stats) const {
    json j;

    // Helper: save shared or phase-dependent values.
    auto save_phase_value = [](const auto &mg, const auto &eg) -> json { return {{"early_game", mg}, {"end_game", eg}}; };

    // Piece values (shared)
    j["piecevals"] = {{"king", params.shared.piecevals[pieces::king]},     {"queen", params.shared.piecevals[pieces::queen]},
                      {"rook", params.shared.piecevals[pieces::rook]},     {"bishop", params.shared.piecevals[pieces::bishop]},
                      {"knight", params.shared.piecevals[pieces::knight]}, {"pawn", params.shared.piecevals[pieces::pawn]}};

    // Mobility values
    j["movevals"] = {{"king", save_phase_value(params.mg.movevals[pieces::king], params.eg.movevals[pieces::king])},
                     {"queen", save_phase_value(params.mg.movevals[pieces::queen], params.eg.movevals[pieces::queen])},
                     {"rook", save_phase_value(params.mg.movevals[pieces::rook], params.eg.movevals[pieces::rook])},
                     {"bishop", save_phase_value(params.mg.movevals[pieces::bishop], params.eg.movevals[pieces::bishop])},
                     {"knight", save_phase_value(params.mg.movevals[pieces::knight], params.eg.movevals[pieces::knight])},
                     {"pawn", save_phase_value(params.mg.movevals[pieces::pawn], params.eg.movevals[pieces::pawn])}};

    // Passed pawn bonuses
    std::array<int, 6> mg_distances;
    std::array<int, 6> eg_distances;

    for (size_t i = 0; i < 6; ++i) {
        mg_distances[i] = params.mg.passed_dist_to_promotion[i + 1];
        eg_distances[i] = params.eg.passed_dist_to_promotion[i + 1];
    }

    j["passed_dist_to_promotion"] = save_phase_value(mg_distances, eg_distances);

    // Scalar parameters
    json scalars = json::object();

#define SAVE_SCALAR_PARAM(name) scalars[#name] = save_phase_value(params.mg.name, params.eg.name);

    EVAL_SCALAR_PARAMS(SAVE_SCALAR_PARAM)

#undef SAVE_SCALAR_PARAM

    j["scalars"] = scalars;

    // Piece-square tables
    json psts = json::object();

    static const std::array<const char *, EvalParameters::num_pieces> piece_names = {"", "king", "queen", "rook", "knight", "bishop", "pawn"};

    auto save_pst = [&](const PieceSquareTable &pst, Piece_t piece, const std::string &key, const std::string &stage) {
        psts[key] = {
            {"piece", std::string(1, Piece::white_pieces[piece])}, {"symmetry", symmetry_name(pst.symmetry())}, {"stage", stage}, {"values", pst.params}};
    };

    for (Piece_t p = 1; p < EvalParameters::num_pieces; ++p) {
        save_pst(params.mg.psts[p], p, std::string(piece_names[p]), "early_game");
        save_pst(params.eg.psts[p], p, std::string(piece_names[p]) + "_end", "end_game");
    }

    j["piece_square_tables"] = psts;

    // Optimization metadata
    j["stats"] = stats;

    // Write JSON
    std::ofstream file(filename);

    if (!file.is_open())
        throw std::runtime_error("Could not open parameter file for writing: " + filename);

    file << j.dump(4) << '\n';

    if (!file.good())
        throw std::runtime_error("Error writing parameter file: " + filename);
}

} // namespace Eval::HCE