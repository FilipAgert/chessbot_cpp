#include <fstream>
#include <string>
#include <map>

class ConfigParser {
private:
    // Structure: [Section] -> {Key: Value}
    std::map<std::string, std::map<std::string, std::string>> data;

public:
    bool load(const std::string& filename);

    std::string read_string(const std::string& key);
    int read_int(const std::string& key, int default_val = 0);
};