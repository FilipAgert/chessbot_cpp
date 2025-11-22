#ifndef BITBOARD_H
#define BITBOARD_H
#include <cstdint>
#include <bitset>
#include <array>
#include <iostream>
#include <integer_representation.h>
#include <string>
#include <sstream>
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
    };
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
    };
   
    constexpr uint64_t one_high(int sq) { return 1ULL << sq; } 
    inline constexpr int bitcount(uint64_t bb){return __builtin_popcountll(bb);};
    inline constexpr int lsb(uint64_t bb){return __builtin_ctzll(bb);};
   
    // print bitboard for debugging
    inline std::string bb_str(uint64_t bb) {std::bitset<64> b(bb); return b.to_string();};
    inline void print(uint64_t bb) { std::cout << bb_str(bb)<< "\n"; };

    std::string to_string_bb(uint64_t bb);
    void print_full(uint64_t bb) { std::cout << BitBoard::to_string_bb(bb)<< "\n"; };

    /**
     * @brief Gets the bitboard for the available knight moves.
     * 
     * @param knight_loc Location of knights.
     * @param friendly_bb location of friendly pieces.
     * @return uint64_t Bitboard of all the knight moves.
     */
    uint64_t knight_moves(const uint64_t knight_bb, const uint64_t friendly_bb);
    uint64_t rook_moves(const uint64_t rook_bb, const uint64_t friendly_bb, const uint64_t enemy_bb);
    uint64_t bishop_moves(const uint64_t bishop_bb, const uint64_t friendly_bb, const uint64_t enemy_bb);
    uint64_t queen_moves(const uint64_t queen_bb, const uint64_t friendly_bb, const uint64_t enemy_bb);
    uint64_t king_moves(const uint64_t king_bb, const uint64_t friendly_bb);
    uint64_t pawn_moves(const uint64_t pawn_bb, const uint64_t friendly_bb, const uint64_t enemy_bb, const uint64_t ep_bb, const uint8_t pawn_color);
    
    /**
     * @brief Shoots ray from origin, up to edge of board and returns the corresponding bitboard.
     * 
     * @param origin 
     * @param dir 
     * @return uint64_t 
     */
    uint64_t ray(const uint64_t origin, const int dir, const uint64_t blocker_bb, const int steps);
    uint64_t ray(const uint64_t origin, const int dir, const uint64_t blocker_bb);
    uint64_t ray(const uint64_t origin, const int dir);

    
    static inline constexpr uint64_t bb_from_array(std::array<uint8_t, 64> arr){
        uint64_t bb = 0;
        for (int i = 0; i<64; i++) {
            if(arr[i] >0) bb |= 1ULL <<i;
        }
        return bb;
    };
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
    static constexpr uint64_t fill = 0xFFFFFFFFFFFFFFFF;
    
    static inline constexpr uint64_t row(uint8_t r){return bottom << 8*r;};//mask for row.
    static inline constexpr uint64_t col(uint8_t c){return left << c;};//mask for col.

    uint64_t edge_mask(int dir);

}


#endif