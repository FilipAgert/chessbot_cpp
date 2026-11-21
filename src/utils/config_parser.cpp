#include "utils/config_parser.h"

#include <fstream>

bool ConfigParser::load(const std::string &filename) {
    std::ifstream file(filename);

    if (!file.is_open())
        return false;

    try {
        file >> data;
    } catch (const json::parse_error &) {
        return false;
    }

    return data.is_object();
}

std::string ConfigParser::read_string(const std::string &key, const std::string &default_val) const {
    if (!data.contains(key) || !data.at(key).is_string())
        return default_val;

    return data.at(key).get<std::string>();
}

int ConfigParser::read_int(const std::string &key, int default_val) const {
    if (!data.contains(key) || !data.at(key).is_number_integer())
        return default_val;

    return data.at(key).get<int>();
}

bool ConfigParser::read_bool(const std::string &key, bool default_val) const {
    if (!data.contains(key) || !data.at(key).is_boolean())
        return default_val;

    return data.at(key).get<bool>();
}

ConfigSettings ConfigParser::get_settings() const {
    ConfigSettings settings;

    settings.aspiration_enable = data.value("aspiration", json::object()).value("enable", false);

    settings.should_ponder = data.value("ponder", json::object()).value("enable", false);

    settings.parameter_file = data.value("eval", json::object()).value("parameter_file", std::string("config/parameters.json"));

    return settings;
}
