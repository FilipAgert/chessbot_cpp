// Copyright 2025 Filip Agert
#ifndef CONFIG_H
#define CONFIG_H

#include <string>
inline std::string const ID_name = "filip_bot";
inline std::string const ID_author = "filipagert";
inline std::string const ID_version = "0.2";

constexpr int STANDARD_TIME = 60 * 1000;  // 60 seconds. 60 * 5 * 1000;  // 5 minutes
constexpr int STANDARD_TINC = 0;          // 0 seconds additional per move.
constexpr int STANDARD_TIME_BUFFER = 10;  // 50 ms buffer to aim for.
constexpr int STANDARD_TIME_FRAC = 25;    // use 1/40th of remanining itme
#endif
