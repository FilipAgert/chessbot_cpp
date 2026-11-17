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
/**
 * @brief For ordering moves.
 * 1. TT moves always go first
 * 2. Loud move: Static exchange evaluation > 0. Sorted by SEE.
 * 3. Loud move: SEE = 0.
 * 4. Killer moves.
 * 5. Quiet SAFE MOVES (See >= 0) (Sorted by history).
 * 6. Loud moves: SEE < 0.
 * 7. Quiet unsafe moves (See < 0)
 *
 * Within bucket 2&3, we sort higher if its a killer move, even if see score is smaller.
 * Bucket 5. sorted by history
 * BUcket 6. sorted by see score
 * Bucket 7. sorted by history.
 * This is solved by
 */
struct MoveScore {
    uint32_t key;
    static constexpr uint8_t TT = 0b10000000;
    static constexpr uint8_t SEE = 0b01000000;
    static constexpr uint8_t SEE_NEUTRAL = 0b00100000;
    static constexpr uint8_t KILLER = 0b00010000;
    static constexpr uint8_t QUIET_SAFE = 0b00001000;
    static constexpr uint8_t SEE_BAD = 0b00000100;
    static constexpr uint8_t QUIET_UNSAFE = 0b00000000;

    MoveScore(int16_t score, uint8_t movetype) : key((uint32_t(movetype) << 16) | uint16_t(score ^ 0x8000)) {}
    MoveScore() : key(0) {}
    int16_t score() const { return int16_t((key & 0xFFFF) ^ 0x8000); }
    uint8_t movetype() const { return key >> 16; }

    /**
     * @brief Sorts by type, then score.
     *
     * @param o
     * @return true
     * @return false
     */
    bool operator<(MoveScore o) const { return key < o.key; }
    bool operator>(MoveScore o) const { return key > o.key; }
};
static const std::array<uint16_t, 7> moveorder_piecevals = {0, std::numeric_limits<int16_t>::max() / 2, 900, 500, 300, 300, 100};
/**
 * @brief Rate move by Heuristics
 *
 * @param[in] move move to be rated
 * @param[in] board board
 * @return score of move to sort by
 */
template <bool is_white> MoveScore move_heuristics(const std::array<int, 7> &piecevals, Move &move, Board &board, BB enemy_pawn_atk_bb, const History &history);
/**
 * @brief Gets true if move is in killer moves.
 *
 * @param move
 * @param killers
 * @return true
 * @return false
 */
bool in_killers(const Move move, const std::array<Move, NUM_KILLER_MOVES> &killers);
template <bool is_white>
void move_heuristics(const std::array<int, 7> &piecevals, std::array<Move, max_legal_moves> &moves, std::array<MoveScore, max_legal_moves> &scores,
                     size_t num_moves, Board &board, std::optional<Move> tt_move, const History &history, const std::array<Move, NUM_KILLER_MOVES> &killers);

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
    inline std::pair<size_t, Move> next() {
        const int i = current_index_++;
        if (i >= sorted_count_)
            select_best(i);
        return {static_cast<size_t>(i), moves_[i]};
    }

    void select_best(int i);

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
