// Copyright 2025 Filip Agert
#include "board.h"
#include "constants.h"
#include "move.h"
#include <algorithm>
#include <array>
#include <moveorder.h>
#include <stdexcept>
#include <unordered_map>

bool MoveOrder::in_killers(const Move move, const std::array<Move, NUM_KILLER_MOVES> &killers) { return (move == killers[0]) || (move == killers[1]); }

namespace MoveOrder {
template <bool is_white> int16_t see(int to, Board &board) {
    // here: Gather all friendly pieces which can attack this one.
    std::optional<Move> move = board.get_smallest_attacking_move<is_white, pieces::pawn>(to);
    int16_t value = 0;
    if (move) {
        assert(move.value().is_valid());
        RestoreMoveInfo info = board.do_move<is_white>(move.value());

        uint8_t captured = move.value().flag != moveflag::MOVEFLAG_pawn_ep_capture ? info.captured : pieces::pawn;
        assert(info.captured <= 6);
        uint8_t promoscore = move.value().is_promotion() ? moveorder_piecevals[move.value().get_promotion()] - moveorder_piecevals[pieces::pawn] : 0;
        value = std::max(0, moveorder_piecevals[captured] + promoscore - see<!is_white>(to, board));
        board.undo_move<is_white>(info, move.value());
    }
    return value;
}
template <bool is_white> std::pair<int16_t, bool> see(Move move, Board &board) {
    bool is_loud = !board.is_move_quiet(move);
    if (!is_loud)
        return {0, false};
    RestoreMoveInfo info = board.do_move<is_white>(move);
    // TODO: Do not build hash delta here. maybe template parameter.
    uint8_t captured = move.flag != moveflag::MOVEFLAG_pawn_ep_capture ? info.captured : pieces::pawn;

    uint8_t promoscore = move.is_promotion() ? moveorder_piecevals[move.get_promotion()] - moveorder_piecevals[pieces::pawn] : 0;
    int16_t value = moveorder_piecevals[captured] + promoscore - see<!is_white>(move.target, board);
    board.undo_move<is_white>(info, move);
    return {value, true};
}

template <bool is_white>
void move_heuristics(const std::array<int, 7> &piecevals, std::array<Move, max_legal_moves> &moves, std::array<MoveScore, max_legal_moves> &scores,
                     size_t num_moves, Board &board, std::optional<Move> tt_move, const History &history, const std::array<Move, NUM_KILLER_MOVES> &killers) {
    for (size_t m = 0; m < num_moves; m++) {
        Move move = moves[m];
        if (tt_move && move == *tt_move) {
            scores[m] = {0, MoveScore::TT};
            continue;
        }
        int16_t score = 0;
        uint8_t flag;
        auto [SEE, is_loud] = see<is_white>(moves[m], board);
        if (is_loud) {
            if (SEE > 0)
                flag = MoveScore::SEE;
            else if (SEE == 0)
                flag = MoveScore::SEE_NEUTRAL;
            else
                flag = MoveScore::SEE_BAD;
            score = SEE;
        } else {
            score = history.get<is_white>(board.get_piece_at(move.source).get_type(), move.target);
            if (SEE >= 0) {
                flag = MoveScore::QUIET_SAFE;
            } else {
                flag = MoveScore::QUIET_UNSAFE;
            }
            if (in_killers(moves[m], killers)) {
                flag |= MoveScore::KILLER;
                // or equal so we can order with killer move within the respective move type.
            }
        }
        scores[m] = {
            score,
            flag,
        };
    }
}
template <bool ascending>
void partial_move_sort(std::array<Move, max_legal_moves> &moves, std::array<MoveOrder::MoveScore, max_legal_moves> &scores, size_t start, size_t num_moves) {
    if (start >= num_moves)
        return;

    std::array<size_t, max_legal_moves> indices;
    std::ranges::iota(indices.begin() + start, indices.begin() + num_moves, start);

    std::sort(indices.begin() + start, indices.begin() + num_moves, [&](size_t a, size_t b) {
        if constexpr (ascending)
            return scores[a] < scores[b];
        else
            return scores[a] > scores[b];
    });

    std::array<Move, max_legal_moves> temp_moves;
    std::array<MoveScore, max_legal_moves> temp_scores;

    for (size_t i = start; i < num_moves; ++i) {
        temp_moves[i] = moves[indices[i]];
        temp_scores[i] = scores[indices[i]];
    }

    for (size_t i = start; i < num_moves; ++i) {
        moves[i] = temp_moves[i];
        scores[i] = temp_scores[i];
    }
}
void MovePicker::select_best(int i) {
    auto first = scores_.begin() + i;
    auto it = std::max_element(first, scores_.begin() + num_moves_);
    int best = static_cast<int>(it - scores_.begin());
    if (best != i) {
        std::swap(moves_[i], moves_[best]);
        std::swap(scores_[i], scores_[best]);
    }
    sorted_count_ = i + 1;
}

void MovePicker::reset_head() { current_index_ = 0; }

void MovePicker::reset_head_sort() {
    current_index_ = 0;
    sorted_count_ = 0;
}

int MovePicker::processed_count() const { return current_index_; }

void MovePicker::sort() {
    partial_move_sort<false>(moves_, scores_, num_moves_, sorted_count_);
    sorted_count_ = num_moves_;
}

template void move_heuristics<true>(const std::array<int, 7> &piecevals, std::array<Move, max_legal_moves> &moves,
                                    std::array<MoveScore, max_legal_moves> &scores, size_t num_moves, Board &board, std::optional<Move> tt_move,
                                    const History &history, const std::array<Move, NUM_KILLER_MOVES> &killers);
template void move_heuristics<false>(const std::array<int, 7> &piecevals, std::array<Move, max_legal_moves> &moves,
                                     std::array<MoveScore, max_legal_moves> &scores, size_t num_moves, Board &board, std::optional<Move> tt_move,
                                     const History &history, const std::array<Move, NUM_KILLER_MOVES> &killers);
}  // namespace MoveOrder