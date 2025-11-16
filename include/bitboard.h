#ifndef BITBOARD_H
#define BITBOARD_H
#include <cstdint>
namespace BitBoard{
    /**
     * @brief Shifts the bitboard
     * 
     * @param board 
     * @return uint64_t 
     */
    inline uint64_t move(const uint64_t board, const uint8_t dir, const uint8_t steps);
};


#endif