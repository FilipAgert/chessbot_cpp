#pragma once
#include "core/board.h"
namespace Eval::HCE::Utils {
/**
 * @brief https://chessprogramming.org/Tapered_Eval
 *
 * @return phase in [0-256] where 0 is early game, 256 is late game. (king+pawns)
 */
template <bool one_sided = false, bool is_white = false> int get_phase(const Board &board);

/**
 * @brief From phase, evaluate score in tapered eval.
 *
 * @tparam one_sided
 * @param phase
 * @param early_game_score
 * @param late_game_score
 * @return int
 */
template <bool one_sided = false> int eval_phase_score(int phase, int early_game_score, int late_game_score);

/**
 * @brief Get manhattan distance between two squares
 *
 * @param[in] sq1 square1
 * @param[in] sq2 square2
 * @return manhattan distance between squares
 */
uint8_t manhattan(uint8_t sq1, uint8_t sq2);

/**
 * @brief Precomputed distance to centre of board in manhattan distance.
 *
 */
extern const std::array<uint8_t, 64> dist2centre;
} // namespace Eval::HCE::Utils