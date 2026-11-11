#pragma once
namespace Parameters {
inline int const DELTA_THRESHHOLD = 200;  // safety margin for delta pruning.
inline int const ASPIRATION_WINDOW = 25;

inline const float REDUCTION_OFFSET = 0.75;
inline const float REDUCTION_MULT = 1. / 2.25;

inline int const NULLMOVE_R = 3;

inline const int MC_R = 4;  // Reduced search depth.
inline const int MC_C = 3;  // Counter. If num moves >= beta in first M moves, prune branch.
inline const int MC_M = 6;  // M > C, number of moves to check

};  // namespace Parameters