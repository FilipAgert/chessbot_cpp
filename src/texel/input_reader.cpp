#include "texel/input_reader.h"
#include "texel/core.h"
#include "utils/file.h"
#include "utils/str.h"
#include <optional>
#include <stdexcept>
namespace Texel {
std::vector<Position> read_dataset(const std::string &filename, std::optional<int> num_lines) {
    std::vector<std::string> lines;
    bool read = Utils::read_lines(filename, lines, num_lines);
    if (!read) {
        throw std::runtime_error("Cannot open '" + filename + "'");
    }

    std::vector<Position> positions;
    positions.reserve(lines.size());

    for (const std::string &line : lines) {
        std::vector<std::string> split = Utils::split(Utils::trim(line), DEL);
        if (split.size() < 2 || split.size() > 3) {
            throw std::runtime_error("Error reading line '" + line + "': should be separated into 2 or 3 parts.");
        }

        std::optional<int> res = Utils::try_process_int(Utils::trim(split[1]));
        if (!res) {
            throw std::runtime_error("Could not process '" + split[1] + "' as int");
        }

        int result = res.value();
        if (abs(result) > 1) {
            throw std::runtime_error("Expected result in range [-1, 1] but was " + split[1]);
        }

        std::optional<double> eval_p = Utils::try_process_double(Utils::trim(split[2]));
        std::optional<int> eval_cp = eval_p.transform([](double a) -> int { return a * 100; });
        positions.push_back(Position{std::move(split[0]), static_cast<Result>(result), eval_cp});
    }
    return positions;
}
}  // namespace Texel