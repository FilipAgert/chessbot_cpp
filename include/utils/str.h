#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Utils {

std::vector<std::string> split(const std::string &full, char del);
/**
 * @brief Trim whitespaces from a string
 *
 * @param[[TODO:direction]] full [TODO:description]
 */
std::string trim(std::string full);
/**
 * @brief Join strings in vector into full string with deliminiter.
 *
 * @param strings
 * @param del
 * @return std::string
 */
std::string join(std::vector<std::string> strings, char del);
std::optional<int> try_process_int(std::string intstring);
}  // namespace Utils