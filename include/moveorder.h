#ifndef MOVEORDER_H
#define MOVEORDER_H

#include "constants.h"
#include "eval.h"
#include <board.h>
#include <history.h>
#include <move.h>
#include <optional>
#include <utility>
namespace MoveOrder {
struct MoveScore {
    int score;
    enum MoveType {
        Capture,
        Killer,
        TT,
        Normal,
    };
    MoveType movetype : 8;
};
constexpr int TT_MOVE_SCORE = 2'000'000'000;
constexpr int CAPTURE_SCORE = 10'000;
constexpr int KILLER_MOVE_SCORE = 10000;
/**
 * @brief Rate move by Heuristics
 *
 * @param[in] move move to be rated
 * @param[in] board board
 * @return score of move to sort by
 */
template <bool is_white> MoveScore move_heuristics(Move &move, Board &board, BB enemy_pawn_atk_bb, const History &history) {
    int score = 0;
    MoveScore::MoveType type = MoveScore::Normal;
    Piece_t pt = board.get_piece_at(move.source).get_type();
    if (move.flag == moveflag::MOVEFLAG_pawn_ep_capture) {
        score += CAPTURE_SCORE * (PieceValue::piecevals[pieces::pawn] * 10 - PieceValue::piecevals[pieces::pawn]);
        type = MoveScore::Capture;
    } else if (!board.is_square_empty(move.target)) {
        score += CAPTURE_SCORE * (PieceValue::piecevals[board.get_piece_at(move.target).get_type()] * 10 - PieceValue::piecevals[pt] +
                                  PieceValue::piecevals[move.get_promotion()]);
        type = MoveScore::Capture;

    } else {
        score += history.get<is_white>(pt, move.target);
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
bool in_killers(const Move &move, const std::array<Move, NUM_KILLER_MOVES> &killers);
template <bool is_white>
void move_heuristics(std::array<Move, max_legal_moves> &moves, std::array<MoveScore, max_legal_moves> &scores, size_t num_moves, Board &board,
                     std::optional<Move> tt_move, const History &history, const std::array<Move, NUM_KILLER_MOVES> &killers) {
    BB enemy_pawn_atk_bb = movegen::pawn_atk_bb<!is_white>(board.get_piece_bb<pieces::pawn, !is_white>());
    for (size_t m = 0; m < num_moves; m++) {
        MoveScore move_score = move_heuristics<is_white>(moves[m], board, enemy_pawn_atk_bb, history);
        scores[m] = move_score;
        if (tt_move && moves[m] == *tt_move) {
            scores[m] = {TT_MOVE_SCORE, MoveScore::TT};
        } else if (move_score.movetype != MoveScore::Capture && in_killers(moves[m], killers)) {
            scores[m] = {KILLER_MOVE_SCORE, MoveScore::Killer};
        }
    }
}

class MovePicker {
 private:
    std::array<Move, max_legal_moves> &moves_;
    std::array<MoveScore, max_legal_moves> &scores_;
    int num_moves_;
    int current_index_;
    int sorted_count_;

 public:
    MovePicker(std::array<Move, max_legal_moves> &moves, std::array<MoveScore, max_legal_moves> &scores, int num_moves)
        : moves_(moves), scores_(scores), num_moves_(num_moves), current_index_(0), sorted_count_(0) {}

    inline bool has_next() const { return current_index_ < num_moves_; }

    /**
     * @brief Gets next move in list sorted by score
     *
     * @return std::pair<size_t, Move&>  index of and move.
     */
    std::pair<size_t, Move &> next();

    /**
     * @brief Reset counter to 0
     *
     */
    void reset_head();

    /**
     * @brief Reset counter to zero and do not trust sort.
     *
     */
    void reset_head_sort();

    /**
     * @brief Gets how many moves processed so far.
     *
     * @return int
     */
    int processed_count() const;

    /**
     * @brief Fully sorts the list.
     *
     */
    void sort();
};

};  // namespace MoveOrder
#endif
