// Copyright 2025 Filip Agert
#include <algorithm>
#include <eval.h>
#include <integer_representation.h>
#include <moveorder.h>
#include <vector>
void MoveOrder::partial_move_sort(std::array<Move, max_legal_moves> &moves,
                                  std::array<int, max_legal_moves> &scores, size_t start,
                                  size_t num_moves, bool ascending) {
    std::vector<std::pair<Move, int>> zipped;
    std::array<size_t, max_legal_moves> indices;
    std::ranges::iota(indices.begin() + start, indices.begin() + num_moves, 0);

    std::sort(indices.begin() + start, indices.begin() + num_moves, [&](size_t a, size_t b) {
        if (ascending)
            return scores[a] < scores[b];
        else
            return scores[a] > scores[b];
    });
    std::array<Move, max_legal_moves> sorted_moves;
    for (size_t i = start; i < num_moves; ++i) {
        sorted_moves[i] = moves[indices[i]];
    }

    std::copy(sorted_moves.begin() + start, sorted_moves.begin() + num_moves,
              moves.begin() + start);
}

int MoveOrder::move_heuristics(Move &move, Board &board) {
    int heuristics = 0;
    if (!board.is_square_empty(move.target)) {
        heuristics += PieceValue::piecevals[board.get_piece_at(move.target).get_type()] * 10 -
                      PieceValue::piecevals[board.get_piece_at(move.source).get_type()];
    }

    if (move.promotion.get_type() > 0) {
        heuristics += PieceValue::piecevals[move.promotion.get_type()];
    }

    uint8_t col = pieces::color_mask ^ board.get_turn_color();  // gets enemy color
    BB pawn_atk_bb = movegen::pawn_atk_bb(board.get_bb(pieces::pawn | col), col);
    if ((BitBoard::one_high(move.target) & pawn_atk_bb) > 0) {
        heuristics -= PieceValue::piecevals[board.get_piece_at(move.source)
                                                .get_type()];  // penalize if target square is on a
                                                               // square defended by a pawn.
    }

    return heuristics;
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
void MoveOrder::apply_move_sort(std::array<Move, max_legal_moves> &moves, size_t num_moves,
                                std::optional<Move> firstmove, Board &board) {
    if (firstmove) {
        std::array<int, max_legal_moves> move_scores;
        int firstmoveidx = -1;
        Move first = firstmove.value();
        for (int m = 0; m < num_moves; m++) {
            int move_score = move_heuristics(moves[m], board);
            if (moves[m].source == first.source && moves[m].target == first.target)
                m = firstmoveidx;
            move_scores[m] = move_score;
        }
        if (firstmoveidx == -1)
            throw std::runtime_error("firstmove was not found in movelist");
        std::swap(move_scores[0], move_scores[firstmoveidx]);
        std::swap(moves[0], moves[firstmoveidx]);
        partial_move_sort(moves, move_scores, firstmoveidx, num_moves, false);
    } else {
        apply_move_sort(moves, num_moves, board);
    }
}
