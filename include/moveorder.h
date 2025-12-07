#ifndef MOVEORDER_H
#define MOVEORDER_H

#include <board.h>
#include <move.h>
class MoveOrder {
 public:
    /**
     * @brief Sorts the moves in array by the scores in scores
     *
     * @param[[TODO:direction]] moves [TODO:description]
     * @param[[TODO:direction]] scores [TODO:description]
     * @param[[TODO:direction]] num_moves [TODO:description]
     * @param[in] ascending: bool to sort in ascending order. If false sorts by descending order.
     * True if minimiser, false if maximiser.
     */
    static void partial_move_sort(std::array<Move, max_legal_moves> &moves,
                                  std::array<int, max_legal_moves> &scores, size_t num_moves,
                                  bool ascending);

    /**
     * @brief Sorts moves by heuristics.
     *
     * @param[[TODO:direction]] moves [TODO:description]
     * @param[[TODO:direction]] board [TODO:description]
     */
    void apply_move_sort(std::array<Move, max_legal_moves> &moves, size_t num_moves, Board &board);

    /**
     * @brief Rate move by Heuristics
     *
     * @param[in] move move to be rated
     * @param[in] board board
     * @return score of move to sort by
     */
    int move_heuristics(Move &move, Board &board);
};
#endif
