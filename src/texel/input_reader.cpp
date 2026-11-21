#include "texel/input_reader.h"
#include "texel/core.h"
#include "utils/file.h"
#include "utils/str.h"

#include <filesystem>
#include <optional>
#include <stdexcept>

namespace Texel {

namespace {

Position parse_fen_line(const std::string &line) {
    std::vector<std::string> split = Utils::split(Utils::trim(line), DEL);

    if (split.size() < 2 || split.size() > 3) {
        throw std::runtime_error("Error reading FEN line '" + line + "': should be separated into 2 or 3 parts.");
    }

    std::optional<int> res = Utils::try_process_int(Utils::trim(split[1]));

    if (!res) {
        throw std::runtime_error("Could not process '" + split[1] + "' as int");
    }

    int result = res.value();

    if (std::abs(result) > 1) {
        throw std::runtime_error("Expected result in range [-1, 1] but was " + split[1]);
    }

    std::optional<int> eval_cp;

    if (split.size() == 3) {
        std::optional<double> eval_p = Utils::try_process_double(Utils::trim(split[2]));

        if (eval_p) {
            eval_cp = static_cast<int>(*eval_p * 100);
        }
    }

    return Position{std::move(split[0]), static_cast<Result>(result), eval_cp};
}

Position parse_epd_line(const std::string &line) {
    std::vector<std::string> parts = Utils::split(line, '"');

    std::string fen = Utils::trim(parts.at(0));
    size_t last_space = fen.find_last_of(' ');
    if (last_space != std::string::npos && fen.substr(last_space + 1) == "c9") {
        fen.replace(last_space + 1, 2, "0");
    }

    std::string score = Utils::trim(parts.at(1));

    // Remove closing quote and optional EPD terminator.
    if (!score.empty() && score.back() == '"') {
        score.pop_back();
    }

    score = Utils::trim(score);

    if (!score.empty() && score.back() == ';') {
        score.pop_back();
    }

    Result result;

    if (score == "1/2-1/2") {
        result = static_cast<Result>(0);
    } else if (score == "0-1") {
        result = static_cast<Result>(-1);
    } else if (score == "1-0") {
        result = static_cast<Result>(1);
    } else {
        throw std::runtime_error("Unknown EPD score '" + score + "' in line '" + line + "'");
    }

    return Position{std::move(fen), result, std::nullopt};
}

} // anonymous namespace

std::vector<Position> read_dataset(const std::string &filename, std::optional<int> num_lines) {
    std::vector<std::string> lines;

    bool read = Utils::read_lines(filename, lines, num_lines);

    if (!read) {
        throw std::runtime_error("Cannot open '" + filename + "'");
    }

    std::string extension = std::filesystem::path(filename).extension().string();

    std::vector<Position> positions;
    positions.reserve(lines.size());

    if (extension == ".fen") {
        for (const std::string &line : lines) {
            positions.push_back(parse_fen_line(line));
        }
    } else if (extension == ".epd") {
        for (const std::string &line : lines) {
            positions.push_back(parse_epd_line(line));
        }
    } else {
        throw std::runtime_error("Unsupported dataset extension '" + extension + "'. Expected .fen or .epd.");
    }

    return positions;
}

} // namespace Texel
