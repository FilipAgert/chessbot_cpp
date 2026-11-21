#include "texel/hce_create_params.h"
#include "eval/hce/piece_square_table.h"

namespace Texel {

HCEParameterCreator::HCEParameterCreator(Eval::HCE::EvalParameters p0, TuningConfig config) : p0(p0), config(config) {}

Eval::HCE::EvalParameters HCEParameterCreator::create_params(const pagmo::vector_double &x) const {

    Eval::HCE::EvalParameters copy = p0;

    size_t index = 0;

    for (const auto &param : config.get_parameter_order()) {
        const std::string &name = param.name;

        // Scalars
#define ASSIGN_SCALAR(name_)                                                                                                                                   \
    if (name == "scalars." #name_) {                                                                                                                           \
        copy.name_ = static_cast<int>(x[index++]);                                                                                                             \
        continue;                                                                                                                                              \
    }

        EVAL_SCALAR_PARAMS(ASSIGN_SCALAR)

#undef ASSIGN_SCALAR

        // Passed pawn distances
        if (name.find("passed_dist_to_promotion.") == 0) {
            size_t i = std::stoul(name.substr(name.find_last_of('.') + 1));
            copy.passed_dist_to_promotion.at(i) = static_cast<int>(x[index++]);
            continue;
        }

        // Piece values and move values
        bool found_piece = false;
        for (const auto &field : {"piecevals", "movevals"}) {
            if (name.find(std::string(field) + ".") != 0)
                continue;

            std::string piece_name = name.substr(std::string(field).size() + 1);

            for (size_t i = pieces::king; i <= pieces::pawn; ++i) {
                if (piece_name != Piece::str_from_piece_type(i))
                    continue;

                if (field == std::string("piecevals"))
                    copy.piecevals[i] = static_cast<int>(x[index++]);
                else
                    copy.movevals[i] = static_cast<int>(x[index++]);
                found_piece = true;
                break;
            }
        }
        if (found_piece)
            continue;

        // Piece square tables
        if (name.find("piece_square_tables.") == 0) {
            const std::string pst_name = name.substr(std::string("piece_square_tables.").size());

            const auto &pst = config.get_settings().at("piece_square_tables").at(pst_name);

            Piece_t piece = Piece::piece_type_from_char(pst.at("piece").get<std::string>().at(0));

            auto symmetry = Eval::HCE::symmetry_from_name(pst.value("symmetry", "NONE"));

            std::string stage = pst.at("stage").get<std::string>();

            std::vector<int> values;
            values.reserve(param.count);

            for (size_t i = 0; i < param.count; ++i)
                values.push_back(static_cast<int>(x[index++]));

            if (stage == "early_game")
                copy.set_pst(piece, symmetry, std::move(values));
            else if (stage == "end_game")
                copy.set_pst_endgame(piece, symmetry, std::move(values));
            else
                throw std::runtime_error("Unknown PST stage: " + stage);

            continue;
        }

        throw std::runtime_error("Unknown optimization parameter: " + name);
    }

    if (index != x.size())
        throw std::runtime_error("Optimization vector size mismatch");

    return copy;
}
std::pair<pagmo::vector_double, pagmo::vector_double> HCEParameterCreator::get_bounds() const {

    pagmo::vector_double lower, upper;

    for (const auto &param : config.get_parameter_order()) {
        for (size_t i = 0; i < param.count; ++i) {
            lower.push_back(param.bounds.lower);
            upper.push_back(param.bounds.upper);
        }
    }

    return {lower, upper};
}
pagmo::vector_double::size_type HCEParameterCreator::get_nix() const {
    pagmo::vector_double::size_type count = 0;

    for (const auto &param : config.get_parameter_order())
        count += param.count;

    return count;
}
pagmo::vector_double HCEParameterCreator::get_initial_guess() const {
    pagmo::vector_double guess;

    for (const auto &param : config.get_parameter_order()) {
        const std::string &name = param.name;

        // Scalars
#define ADD_SCALAR(name_)                                                                                                                                      \
    if (name == "scalars." #name_) {                                                                                                                           \
        guess.push_back(p0.name_);                                                                                                                             \
        continue;                                                                                                                                              \
    }

        EVAL_SCALAR_PARAMS(ADD_SCALAR)

#undef ADD_SCALAR

        // Passed pawn distances
        if (name.find("passed_dist_to_promotion.") == 0) {
            size_t i = std::stoul(name.substr(name.find_last_of('.') + 1));
            guess.push_back(p0.passed_dist_to_promotion.at(i));
            continue;
        }

        // Piece values and move values
        bool found_piece = false;
        for (const auto &field : {"piecevals", "movevals"}) {
            if (name.find(std::string(field) + ".") != 0)
                continue;

            std::string piece_name = name.substr(std::string(field).size() + 1);
            for (size_t i = pieces::king; i <= pieces::pawn; ++i) {
                if (piece_name != Piece::str_from_piece_type(i))
                    continue;
                found_piece = true;
                if (field == std::string("piecevals"))
                    guess.push_back(p0.piecevals[i]);
                else
                    guess.push_back(p0.movevals[i]);

                break;
            }
        }
        if (found_piece)
            continue;

        // Piece square tables
        if (name.find("piece_square_tables.") == 0) {
            const std::string pst_name = name.substr(std::string("piece_square_tables.").size());

            const auto &pst = config.get_settings().at("piece_square_tables").at(pst_name);

            Piece_t piece = Piece::piece_type_from_char(pst.at("piece").get<std::string>().at(0));

            auto symmetry = Eval::HCE::symmetry_from_name(pst.value("symmetry", "NONE"));

            std::string stage = pst.at("stage").get<std::string>();

            const Eval::HCE::PieceSquareTable *table = nullptr;

            if (stage == "early_game") {
                table = &p0.psts.at(piece);
            } else if (stage == "end_game") {
                if (!p0.psts_endgame.at(piece))
                    throw std::runtime_error("Missing initial endgame PST: " + pst_name);

                table = &*p0.psts_endgame.at(piece);
            } else {
                throw std::runtime_error("Unknown PST stage: " + stage);
            }

            auto values = table->params;

            if (values.size() != param.count)
                throw std::runtime_error("PST dimension mismatch: " + pst_name + "want: " + std::to_string(param.count) +
                                         " got: " + std::to_string(values.size()));

            for (int value : values)
                guess.push_back(value);

            continue;
        }

        throw std::runtime_error("Unknown optimization parameter: " + name);
    }

    return guess;
}

} // namespace Texel