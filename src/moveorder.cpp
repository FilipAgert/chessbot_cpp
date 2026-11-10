// Copyright 2025 Filip Agert
#include <algorithm>
#include <moveorder.h>


bool MoveOrder::in_killers(const Move& move, const std::array<Move, NUM_KILLER_MOVES>& killers){
    return (move == killers[0]) || (move == killers[1]);
}




namespace MoveOrder{
    template<bool ascending>
    void partial_move_sort(std::array<Move, max_legal_moves> &moves,
                                  std::array<MoveOrder::MoveScore, max_legal_moves> &scores, 
                                  size_t start,
                                  size_t num_moves) {
        if (start >= num_moves) return;

        std::array<size_t, max_legal_moves> indices;
        std::ranges::iota(indices.begin() + start, indices.begin() + num_moves, start);

        std::sort(indices.begin() + start, indices.begin() + num_moves, [&](size_t a, size_t b) {
            if constexpr (ascending)
                return scores[a].score < scores[b].score;
            else
                return scores[a].score > scores[b].score;
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

    std::pair<size_t, Move&> MovePicker::next() {
        if (current_index_ < sorted_count_) {
            size_t idx = current_index_++;
            return {idx,moves_[idx]};
        }

        //lazily sort.
        int best = current_index_;
        for (int j = current_index_ + 1; j < num_moves_; ++j) {
            if (scores_[j].score > scores_[best].score) {
                best = j;
            }
        }
        
        std::swap(moves_[current_index_], moves_[best]);
        std::swap(scores_[current_index_], scores_[best]);

        // Mark this index as sorted for future passes
        sorted_count_ = current_index_ + 1;
        int idx = current_index_++;
        return {idx, moves_[idx]};
    }
    void MovePicker::reset_head(){
        current_index_ = 0;
    }

    void MovePicker::reset_head_sort(){
        current_index_ = 0;
        sorted_count_ = 0;
    }
    
    int MovePicker::processed_count() const{
        return current_index_;
    }

    void MovePicker::sort(){
        partial_move_sort<false>(moves_, scores_, num_moves_, sorted_count_);
        sorted_count_ = num_moves_;
    }
}