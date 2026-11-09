#ifndef MOVEORDER_H
#define MOVEORDER_H

#include "constants.h"
#include "eval.h"
#include <history.h>
#include <board.h>
#include <move.h>
#include <optional>
#include <utility>
namespace MoveOrder {
struct MoveScore{
    int score;
    enum MoveType{
        Capture,
        Killer,
        TT,
        Normal,
    };
    MoveType movetype:8;

};
constexpr int TT_MOVE_SCORE = 2'000'000'000;
constexpr int CAPTURE_SCORE = 10'000;
constexpr int KILLER_MOVE_SCORE = 10000;
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
template <bool is_white> MoveScore move_heuristics(Move &move, Board &board, BB enemy_pawn_atk_bb, const History& history) {
    int score = 0;
    MoveScore::MoveType type = MoveScore::Normal;
    Piece_t pt = board.get_piece_at(move.source).get_type();
    if (move.flag == moveflag::MOVEFLAG_pawn_ep_capture) {
        score += CAPTURE_SCORE*(PieceValue::piecevals[pieces::pawn] * 10 - PieceValue::piecevals[pieces::pawn]);
        type=MoveScore::Capture;
    } else if (!board.is_square_empty(move.target)) {
        score += CAPTURE_SCORE*(PieceValue::piecevals[board.get_piece_at(move.target).get_type()] * 10
            - PieceValue::piecevals[pt] + PieceValue::piecevals[move.get_promotion()]);
        type = MoveScore::Capture;
        
    } else{
        score += history.get<is_white>(pt,  move.target);
    }
    // if (move.is_promotion()) {
    //     score += PieceValue::piecevals[move.get_promotion()] *10;
    // } commented out since we take history for all non-captures.

    // if ((BitBoard::one_high(move.target) & enemy_pawn_atk_bb) > 0) {
    //     score -= PieceValue::piecevals[pt];  // penalize if target square is on a
    //                                                            // square defended by a pawn.
    // }

    return {score, type};
}
/**
 * @brief Gets true if move is in killer moves.
 * 
 * @param move 
 * @param killers 
 * @return true 
 * @return false 
 */
bool in_killers(const Move& move, const std::array<Move, NUM_KILLER_MOVES>& killers);
template<bool is_white> void move_heuristics(std::array<Move, max_legal_moves>&moves, std::array<MoveScore, max_legal_moves>&scores, size_t num_moves, Board&board, std::optional<Move> tt_move, const History& history, const std::array<Move, NUM_KILLER_MOVES>& killers){
    BB enemy_pawn_atk_bb = movegen::pawn_atk_bb<!is_white>(board.get_piece_bb<pieces::pawn, !is_white>());
    for (size_t m = 0; m < num_moves; m++) {
        MoveScore move_score = move_heuristics<is_white>(moves[m], board, enemy_pawn_atk_bb, history);
        scores[m] = move_score;
        if (tt_move && moves[m] == *tt_move){
            scores[m] = {TT_MOVE_SCORE, MoveScore::TT};
        } else if (move_score.movetype != MoveScore::Capture && in_killers(moves[m], killers)){
            scores[m] = {KILLER_MOVE_SCORE, MoveScore::Killer};
        }
    }
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

    BB enemy_pawn_atk_bb = movegen::pawn_atk_bb<!is_white>(board.get_piece_bb<pieces::pawn, !is_white>());
    for (size_t m = 0; m < num_moves; m++) {
        int move_score = move_heuristics<is_white>(moves[m], board, enemy_pawn_atk_bb);
        move_scores[m] = move_score;
    }
    partial_move_sort(moves, move_scores, num_moves, false);
}

};  // namespace MoveOrder
#endif
