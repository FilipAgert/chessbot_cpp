// Copyright 2025 Filip Agert
#include <algorithm>
#include <eval.h>
#include <moveorder.h>
#include <vector>
void MoveOrder::partial_move_sort(std::array<Move, max_legal_moves> &moves,
                                  std::array<int, max_legal_moves> &scores, size_t num_moves,
                                  bool ascending) {
    std::vector<std::pair<Move, int>> zipped;
    std::array<size_t, max_legal_moves> indices;
    std::ranges::iota(indices.begin(), indices.begin() + num_moves, 0);

    std::sort(indices.begin(), indices.begin() + num_moves, [&](size_t a, size_t b) {
        if (ascending)
            return scores[a] < scores[b];
        else
            return scores[a] > scores[b];
    });
    std::array<Move, max_legal_moves> sorted_moves;
    for (size_t i = 0; i < num_moves; ++i) {
        sorted_moves[i] = moves[indices[i]];
    }

    std::copy(sorted_moves.begin(), sorted_moves.begin() + num_moves, moves.begin());
}

int MoveOrder::move_heuristics(Move &move, Board &board) {
    int piece_cap_eval = 0;
    if (!board.is_square_empty(move.target)) {
        piece_cap_eval = PieceValue::piecevals[board.get_piece_at(move.target).get_type()] * 10 -
                         PieceValue::piecevals[board.get_piece_at(move.source).get_type()];
    }

    int piece_promotion_eval = 0;
    if (move.promotion.get_type() > 0) {
        piece_promotion_eval = PieceValue::piecevals[move.promotion.get_type()];
    }

    return piece_cap_eval + piece_promotion_eval;
}

void MoveOrder::apply_move_sort(std::array<Move, max_legal_moves> &moves, size_t num_moves,
                                Board &board) {
    std::array<int, max_legal_moves> move_scores;
    for (int m = 0; m < num_moves; m++) {
        int move_score = move_heuristics(moves[m], board);
        move_scores[m] = move_score;
    }
    partial_move_sort(moves, move_scores, num_moves, false);
}
