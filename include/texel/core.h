#pragma once
#include <optional>
#include <string>
namespace Texel {
enum class Result { White = 1, Draw = 0, Black = -1 };
struct Position {
    std::string fen;
    Result res;
    std::optional<int> eval;
};
}  // namespace Texel