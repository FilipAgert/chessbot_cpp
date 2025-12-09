#ifndef MOVEORDER_H
#define MOVEORDER_H

#include <board.h>
#include <move.h>
#include <optional>
namespace MoveOrder {
void partial_move_sort(std::array<Move, max_legal_moves> &moves,
                       std::array<int, max_legal_moves> &scores, size_t start, size_t num_moves,
                       bool ascending);

/**
 * @brief Sorts the moves in array by the scores in scores
 *
 * @param[[TODO:direction]] moves [TODO:description]
 * @param[[TODO:direction]] scores [TODO:description]
 * @param[[TODO:direction]] num_moves [TODO:description]
 * @param[in] ascending: bool to sort in ascending order. If false sorts by descending order.
 * True if minimiser, false if maximiser.
 */
void partial_move_sort(std::array<Move, max_legal_moves> &moves,
                       std::array<int, max_legal_moves> &scores, size_t num_moves, bool ascending) {
    partial_move_sort(moves, scores, 0, num_moves, ascending);
}
/**
 * @brief Sorts moves by heuristics.
 *
 * @param[[TODO:direction]] moves [TODO:description]
 * @param[[TODO:direction]] board [TODO:description]
 */
void apply_move_sort(std::array<Move, max_legal_moves> &moves, size_t num_moves, Board &board);
/**
 * @brief Sort moves, and set the firstmove first no matter the other metrics.
 *
 * @param[inout] moves moves
 * @param[in] num_moves number of moves
 * @param[in] firstmove optional first move. will be first in array
 * @param[in] board board
 */
void apply_move_sort(std::array<Move, max_legal_moves> &moves, size_t num_moves,
                     std::optional<Move> firstmove, Board &board);

/**
 * @brief Rate move by Heuristics
 *
 * @param[in] move move to be rated
 * @param[in] board board
 * @return score of move to sort by
 */
int move_heuristics(Move &move, Board &board);
};  // namespace MoveOrder
#endif
