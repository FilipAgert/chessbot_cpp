#ifndef BITBOARD_H
#define BITBOARD_H
#include <cstdint>
#include <bitset>
namespace BitBoard{
    /**
     * @brief Shifts the bitboard in a certain direction n # of steps. Branchless execution
     * 
     * @param board board to shift
     * @param dir direction to move in (N,E,S,W etc)
     * @param steps How many steps to move in this direction
     * @return shifted bitboard
     */
    static inline constexpr uint64_t shift_bb(const uint64_t board, const int dir, const uint8_t steps){
        int left_shift = (dir > 0) * dir*steps;
        int right_shift = (dir < 0) * (-dir)*steps;
        uint64_t left_mask = -(dir > 0);
        uint64_t right_mask = -(dir < 0);
        uint64_t left = (board << left_shift)&left_mask;
        uint64_t right = (board >> right_shift) & right_mask;
        return left | right;
    }
    /**
     * @brief Shifts the bitboard in a certain direction for 1 step. Branchless execution
     * 
     * @param board board to shift
     * @param dir direction to move in (N,E,S,W etc)
     * @return shifted bitboard
     */
    static inline constexpr uint64_t shift_bb(const uint64_t board, const int dir){
        int left_shift = (dir > 0) * dir;
        int right_shift = (dir < 0) * (-dir);
        uint64_t left_mask = -(dir > 0); //Generates mask of all ones, or mask of all 0s.
        uint64_t right_mask = -(dir < 0); 
        uint64_t left = (board << left_shift)&left_mask;
        uint64_t right = (board >> right_shift) & right_mask;
        return left | right;
    }
   
    /**
     * @brief Gets the bitboard for the available knight moves.
     * 
     * @param knight_loc Location of knights.
     * @return uint64_t Bitboard of all the knight moves.
     */
    uint64_t knight_moves(const uint64_t knight_loc);
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

namespace masks{//Containing masks for bitboards. E.g. mask out top row or someth(ing similar.
    static constexpr uint64_t bottom = 0b11111111;
    static constexpr uint64_t top = BitBoard::shift_bb(bottom, dirs::N,7);
    static constexpr uint64_t left =  0x0101010101010101;
    static constexpr uint64_t right =  BitBoard::shift_bb(left, dirs::E, 7);
    static constexpr uint64_t sides = left | right;
    
    static inline constexpr uint64_t row(uint8_t r){return bottom << 8*r;};
    static inline constexpr uint64_t col(uint8_t c){return left << c;};

}


#endif