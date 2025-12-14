// Copyright 2025 Filip Agert
#include <algorithm>
#include <moveorder.h>
#include <vector>
void MoveOrder::partial_move_sort(std::array<Move, max_legal_moves> &moves,
                                  std::array<int, max_legal_moves> &scores, size_t start,
                                  size_t num_moves, bool ascending) {
    std::vector<std::pair<Move, int>> zipped;
    std::array<size_t, max_legal_moves> indices;
    std::ranges::iota(indices.begin() + start, indices.begin() + num_moves, start);

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
