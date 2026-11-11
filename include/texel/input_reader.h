/**
 * @file input_reader.h
 * @author Filip Agert (filip.agert@gmail.com)
 * @brief Just reads the input files of fen.
 * Format: <fen>,<win/draw/loss> as <1/0/-1>
 * @version 0.1
 * @date 2026-11-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once
#include "texel/core.h"
#include <optional>
#include <string>
#include <vector>
namespace Texel {
static const char DEL = ',';
std::vector<Position> read_dataset(const std::string &filename, std::optional<int> num_fens);
}  // namespace Texel