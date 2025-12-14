#ifndef MOVEORDER_H
#define MOVEORDER_H

#include "eval.h"
#include <board.h>
#include <move.h>
#include <optional>
#include <utility>
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
inline void partial_move_sort(std::array<Move, max_legal_moves> &moves,
                              std::array<int, max_legal_moves> &scores, size_t num_moves,
                              bool ascending) {
    partial_move_sort(moves, scores, 0, num_moves, ascending);
}
/**
 * @brief Rate move by Heuristics
 *
 * @param[in] move move to be rated
 * @param[in] board board
 * @return score of move to sort by
 */
template <bool is_white> int move_heuristics(Move &move, Board &board) {
    int heuristics = 0;
    if (!board.is_square_empty(move.target)) {
        heuristics += PieceValue::piecevals[board.get_piece_at(move.target).get_type()] * 10 -
                      PieceValue::piecevals[board.get_piece_at(move.source).get_type()];
    }

    if (move.promotion.get_type() > 0) {
        heuristics += PieceValue::piecevals[move.promotion.get_type()];
    }

    BB pawn_atk_bb = movegen::pawn_atk_bb<is_white>(board.get_piece_bb<pieces::pawn, is_white>());
    if ((BitBoard::one_high(move.target) & pawn_atk_bb) > 0) {
        heuristics -= PieceValue::piecevals[board.get_piece_at(move.source)
                                                .get_type()];  // penalize if target square is on a
                                                               // square defended by a pawn.
    }

    return heuristics;
}
/**
 * @brief Sorts moves by heuristics.
 *
 * @param[[TODO:direction]] moves [TODO:description]
 * @param[[TODO:direction]] board [TODO:description]
 */
template <bool is_white>
void apply_move_sort(std::array<Move, max_legal_moves> &moves, size_t num_moves, Board &board) {
    std::array<int, max_legal_moves> move_scores;
    for (int m = 0; m < num_moves; m++) {
        int move_score = move_heuristics<is_white>(moves[m], board);
        move_scores[m] = move_score;
    }
    partial_move_sort(moves, move_scores, num_moves, false);
}
/**
 * @brief Sort moves, and set the firstmove first no matter the other metrics.
 *
 * @param[inout] moves moves
 * @param[in] num_moves number of moves
 * @param[in] firstmove optional first move. will be first in array
 * @param[in] board board
 */
template <bool is_white>
void apply_move_sort(std::array<Move, max_legal_moves> &moves, size_t num_moves,
                     std::optional<Move> firstmove, Board &board) {
    if (firstmove) {
        std::array<int, max_legal_moves> move_scores;
        int firstmoveidx = -1;
        Move first = firstmove.value();
        for (int m = 0; m < num_moves; m++) {
            int move_score = move_heuristics<is_white>(moves[m], board);
            if (moves[m].source == first.source && moves[m].target == first.target)
                firstmoveidx = m;
            move_scores[m] = move_score;
        }
        if (firstmoveidx == -1) {
            std::cout << firstmove.value().toString() << std::endl;
            throw std::runtime_error("firstmove was not found in movelist");
        }
        std::swap(move_scores[0], move_scores[firstmoveidx]);
        std::swap(moves[0], moves[firstmoveidx]);
        partial_move_sort(moves, move_scores, 1, num_moves, false);
    } else {
        apply_move_sort<is_white>(moves, num_moves, board);
    }
}

};  // namespace MoveOrder
#endif
