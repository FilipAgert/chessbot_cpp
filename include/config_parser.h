#include <fstream>
#include <string>
#include <map>

class ConfigParser {
private:
    // Structure: [Section] -> {Key: Value}
    std::map<std::string, std::map<std::string, std::string>> data;

public:
    bool load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return false;

        std::string line, currentSection = "default";
        while (std::getline(file, line)) {
            // Basic trimming and comment skipping
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            if (line.empty() || line[0] == '#') continue;

            // Handle [Section]
            if (line[0] == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.size() - 2);
            } 
            // Handle key value
            else {
                size_t spacePos = line.find_first_of(" \t=");
                if (spacePos != std::string::npos) {
                    std::string key = line.substr(0, spacePos);
                    std::string value = line.substr(line.find_first_not_of(" \t=", spacePos));
                    data[currentSection][key] = value;
                }
            }
        }
        return true;
    }

    std::string read_string(const std::string& key) {
        size_t slashPos = key.find('/');
        if (slashPos == std::string::npos) return ""; // Invalid format

        std::string section = key.substr(0, slashPos);
        std::string subkey = key.substr(slashPos + 1);

        if (data.count(section) && data[section].count(subkey)) {
            return data[section][subkey];
        }
        return "NOT_FOUND";
    }
    int read_int(const std::string& key, int default_val = 0) {
        size_t slashPos = key.find('/');
        if (slashPos == std::string::npos) return default_val; // Invalid format

        std::string section = key.substr(0, slashPos);
        std::string subkey = key.substr(slashPos + 1);

        if (data.count(section) && data[section].count(subkey)) {
            return std::stoi(data[section][subkey]);
        }
        return default_val;
    }
};