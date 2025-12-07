// Copyright 2025 Filip Agert
#include <algorithm>
#include <eval.h>
#include <moveorder.h>
#include <vector>
void MoveOrder::partial_move_sort(std::array<Move, max_legal_moves> &moves,
                                  std::array<int, max_legal_moves> &scores, size_t num_moves,
                                  bool ascending) {
    std::vector<std::pair<Move, int>> zipped;

    for (size_t i = 0; i < num_moves; i++) {
        int s = scores[i];
        if (!ascending)
            s = s * (-1);
        zipped.push_back({moves[i], s});
    }

    std::sort(zipped.begin(), zipped.end(), [](auto a, auto b) {
        return a.second < b.second;
    });  // Lambda to specify sort by second (score).

    for (size_t i = 0; i < num_moves; i++) {
        moves[i] = zipped[i].first;
        int s = zipped[i].second;
        if (!ascending)
            s = s * (-1);

        scores[i] = s;
    }
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
