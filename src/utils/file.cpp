#include "utils/file.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace Utils {
bool read_lines(const std::string &path, std::vector<std::string> &out, std::optional<int> num_lines) {
    std::ifstream file(path);
    if (!file)
        return false;

    std::string line;
    int num_processed = 0;
    while (std::getline(file, line)) {
        // handle Windows line endings
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        out.push_back(std::move(line));
        if (num_lines && (++num_processed > num_lines.value()))
            break;
    }
    return true;
}

}  // namespace Utils