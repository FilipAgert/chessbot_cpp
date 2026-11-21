#include "eval/hce/parameter_config.h"
#include "eval/hce/parameters.h"
#include "piece_square_table.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace Eval::HCE {
using json = nlohmann::json;

ParameterConfig::ParameterConfig(const std::string &filename) { load(filename); }

void ParameterConfig::load(const std::string &filename) {
    std::ifstream file(filename);
    json j = json::parse(file);
    EvalParameters p;

    auto piecevals = j.value("piecevals", json::object());
    p.piecevals[pieces::king] = piecevals.value("king", 200000);
    p.piecevals[pieces::queen] = piecevals.value("queen", 1009);
    p.piecevals[pieces::rook] = piecevals.value("rook", 570);
    p.piecevals[pieces::bishop] = piecevals.value("bishop", 350);
    p.piecevals[pieces::knight] = piecevals.value("knight", 336);
    p.piecevals[pieces::pawn] = piecevals.value("pawn", 130);

    auto movevals = j.value("movevals", json::object());
    p.movevals[pieces::king] = movevals.value("king", -5);
    p.movevals[pieces::queen] = movevals.value("queen", 12);
    p.movevals[pieces::rook] = movevals.value("rook", 21);
    p.movevals[pieces::bishop] = movevals.value("bishop", 9);
    p.movevals[pieces::knight] = movevals.value("knight", 12);
    p.movevals[pieces::pawn] = movevals.value("pawn", 11);

    const std::array<int, 6> default_dist2promo = {107, 86, 62, 26, -3, -6};
    std::array<int, 6> distances = j.value("passed_dist_to_promotion", default_dist2promo);
    for (size_t i = 0; i < 6; i++) {
        p.passed_dist_to_promotion.at(i + 1) = distances[i];
    }

#define ASSIGN_SCALAR_PARAM(name) p.name = scalars.value(#name, 0);

    // scalar parameter loading
    auto scalars = j.value("scalars", json::object());
    EVAL_SCALAR_PARAMS(ASSIGN_SCALAR_PARAM);

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
            p.set_pst(piece, s, params);
        else if (stage == "end_game")
            p.set_pst_endgame(piece, s, params);
        else
            std::cerr << "error parsing stage: " << stage << std::endl;
    }
    params = p;
}

void ParameterConfig::save(const std::string &filename, const json &stats) const {

    json j;

    // Piece values
    j["piecevals"] = {{"king", params.piecevals[pieces::king]},     {"queen", params.piecevals[pieces::queen]},   {"rook", params.piecevals[pieces::rook]},
                      {"bishop", params.piecevals[pieces::bishop]}, {"knight", params.piecevals[pieces::knight]}, {"pawn", params.piecevals[pieces::pawn]}};

    // Move values
    j["movevals"] = {{"king", params.movevals[pieces::king]},     {"queen", params.movevals[pieces::queen]},   {"rook", params.movevals[pieces::rook]},
                     {"bishop", params.movevals[pieces::bishop]}, {"knight", params.movevals[pieces::knight]}, {"pawn", params.movevals[pieces::pawn]}};

    // Passed pawn distances
    std::array<int, 6> distances;

    for (size_t i = 0; i < 6; i++)
        distances[i] = params.passed_dist_to_promotion.at(i + 1);

    j["passed_dist_to_promotion"] = distances;

    // Scalar parameters
    json scalars = json::object();

#define SAVE_SCALAR_PARAM(name) scalars[#name] = params.name;

    EVAL_SCALAR_PARAMS(SAVE_SCALAR_PARAM);

#undef SAVE_SCALAR_PARAM

    j["scalars"] = scalars;

    // Piece-square tables
    // See the note below about accessing the PST parameter vectors.

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