#pragma once
#include <optional>
#include <string>
#include <vector>
namespace Utils {
bool read_lines(const std::string &path, std::vector<std::string> &out, std::optional<int> num_lines = std::nullopt);
}