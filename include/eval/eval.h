#pragma once
#include "core/board.h"
#include "core/move.h"
#include "eval/hce/parameters.h"
namespace Eval {
constexpr int MATE_SCORE = 30000;

class Eval {
  public:
    virtual const HCE::EvalParameters &get_params() const = 0;
    virtual ~Eval() = default;

    virtual int evaluate(const Board &board) = 0;
    virtual void push_move(Move move, const Board &board) = 0;
    virtual void pop_move(Move move, const Board &board) = 0;
    virtual bool delta_futility_pruning(int standing_pat, int alpha, int threshhold, const Board &board) = 0;
};

/**
 * @brief Checks if draw by ply moves.
 *
 * @param board
 * @return true
 * @return false
 */
bool forced_draw_ply(const Board &board);
/**
 * @brief If score has mate, checks how many moves are left until mate (not plys)
 *
 * @param score
 * @return std::optional<int>
 */
std::optional<int> moves_to_mate(int score);
/**
 * @brief CHecks if draw by insufficient mating material
 *
 * @param board
 * @return true
 * @return false
 */
bool insufficient_mating_material(const Board &board);

template <bool is_white> bool side_cannot_win(const Board &board);
template <bool is_white> bool can_zugzwang(const Board &board) {
    return board.occupancy<is_white>() == (board.get_piece_bb<pieces::king, is_white>() | board.get_piece_bb<pieces::pawn, is_white>());
}
} // namespace Eval