#pragma once

#include "utils/config.h"
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

class ConfigParser {
  private:
    json data;

  public:
    bool load(const std::string &filename);

    std::string read_string(const std::string &key, const std::string &default_val = "") const;

    int read_int(const std::string &key, int default_val) const;
    bool read_bool(const std::string &key, bool default_val) const;

    ConfigSettings get_settings() const;
};
