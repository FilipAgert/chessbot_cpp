#include "utils/str.h"
#include <exception>
#include <sstream>
#include <string>
namespace Utils {
std::vector<std::string> split(const std::string &full, char del) {
    std::stringstream ss(full);
    std::string temp;
    std::vector<std::string> out;
    while (std::getline(ss, temp, del)) {
        out.push_back(temp);
    }
    return out;
}
std::string trim(std::string full) {
    std::string out = "";
    size_t i;
    for (i = 0; i < full.length() && full[i] == ' '; i++) {
    }
    size_t start = i;

    for (i = full.length() - 1; i >= 0 && full[i] == ' '; i--) {
    }
    size_t stop = i;
    if (start <= stop)
        out = full.substr(start, stop - start + 1);
    return out;
}
std::string join(std::vector<std::string> strings, char del) {
    std::string str = "";
    for (size_t i = 0; i < strings.size(); i++) {
        if (i != 0)
            str.push_back(del);
        str.append(strings[i]);
    }
    return str;
}
std::optional<int> try_process_int(std::string intstring) {
    int depth;
    try {
        depth = std::stoi(trim(intstring));
    } catch (const std::exception &e) {
        return {};
    }
    return std::make_optional(depth);
}
std::optional<double> try_process_double(std::string doublestring) {
    int depth;
    try {
        depth = std::stod(trim(doublestring));
    } catch (const std::exception &e) {
        return {};
    }
    return std::make_optional(depth);
}
}  // namespace Utils