#ifndef BITBOARD_H
#define BITBOARD_H
#include <cstdint>
#include <bitset>
namespace BitBoard{
    /**
     * @brief Shifts the bitboard in a certain direction n # of steps.
     * 
     * @param board board to shift
     * @param dir direction to move in (N,E,S,W etc)
     * @param steps How many steps to move in this direction
     * @return shifted bitboard
     */
    inline uint64_t shift_bb(const uint64_t board, const int dir, const uint8_t steps){
        int left_shift = (dir > 0) * dir*steps;
        int right_shift = (dir < 0) * (-dir)*steps;
        uint64_t left_mask = -(dir > 0);
        uint64_t right_mask = -(dir < 0);
        uint64_t left = (board << left_shift)&left_mask;
        uint64_t right = (board >> right_shift) & right_mask;
        return left | right;
    }
    /**
     * @brief Shifts the bitboard in a certain direction for 1 step.
     * 
     * @param board board to shift
     * @param dir direction to move in (N,E,S,W etc)
     * @return shifted bitboard
     */
    inline uint64_t shift_bb(const uint64_t board, const int dir){
        int left_shift = (dir > 0) * dir;
        int right_shift = (dir < 0) * (-dir);
        uint64_t left_mask = -(dir > 0);
        uint64_t right_mask = -(dir < 0);
        uint64_t left = (board << left_shift)&left_mask;
        uint64_t right = (board >> right_shift) & right_mask;
        return left | right;
    }
};

namespace dirs{
    //Directions. Each direction changes an idx by the corresponding amount.
    static constexpr int N = 8;
    static constexpr int E = 1;
    static constexpr int S = -8;
    static constexpr int W = -1;
    static constexpr int SW = S+W;
    static constexpr int NE = N+E;
    static constexpr int SE = S+E;
    static constexpr int NW = N+W;
}


#endif