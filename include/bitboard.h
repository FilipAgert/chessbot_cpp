// Copyright 2025 Filip Agert
#ifndef BITBOARD_H
#define BITBOARD_H
#include <integer_representation.h>

#include <array>
#include <bitset>
#include <cstdint>
#include <iostream>
#include <string>
namespace BitBoard {
/**
 * @brief Shifts the bitboard in a certain direction n # of steps. Branchless execution
 *
 * @param board board to shift
 * @param dir direction to move in (N,E,S,W etc)
 * @param steps How many steps to move in this direction
 * @return shifted bitboard
 */
[[nodiscard("The return value should be handled as a uint64_t.")]]
static inline constexpr uint64_t shift_bb(const uint64_t board, const int dir,
                                          const uint8_t steps) {
    int left_shift = (dir > 0) * dir * steps;
    int right_shift = (dir < 0) * (-dir) * steps;
    uint64_t left_mask = -(dir > 0);
    uint64_t right_mask = -(dir < 0);
    uint64_t left = (board << left_shift) & left_mask;
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
[[nodiscard("The return value should be handled as a uint64_t.")]]
static inline constexpr uint64_t shift_bb(const uint64_t board, const int dir) {
    int left_shift = (dir > 0) * dir;
    int right_shift = (dir < 0) * (-dir);
    uint64_t left_mask = -(dir > 0);  // Generates mask of all ones, or mask of all 0s.
    uint64_t right_mask = -(dir < 0);
    uint64_t left = (board << left_shift) & left_mask;
    uint64_t right = (board >> right_shift) & right_mask;
    return left | right;
}
[[nodiscard("The return value should be handled as a uint64_t.")]]
constexpr uint64_t one_high(uint8_t sq) {
    return 1ULL << sq;
}
inline constexpr int bitcount(uint64_t bb) { return __builtin_popcountll(bb); }
/**
 * @brief Extracts least significant bit location. Rightmost = 0
 *
 * @param[in] bb Bitboard
 * @return lsb index
 */
inline constexpr int lsb(uint64_t bb) { return __builtin_ctzll(bb); }

/**
 * @brief Gets bitboard as a string of 0 and 1.
 */
inline std::string bb_str(uint64_t bb) {
    std::bitset<64> b(bb);
    return b.to_string();
}
inline void print(uint64_t bb) { std::cout << bb_str(bb) << "\n"; }

/**
 * @brief Gets bitboard as a 8x8 square filled with 1s. 0s are empty squares.
 *
 * @param[[TODO:direction]] bb [TODO:description]
 */
std::string to_string_bb(uint64_t bb);
void print_full(uint64_t bb);
[[nodiscard("The return value should be handled as a uint64_t.")]]
static inline constexpr uint64_t bb_from_array(std::array<uint8_t, 64> arr) {
    uint64_t bb = 0;
    for (int i = 0; i < 64; i++) {
        if (arr[i] > 0)
            bb |= 1ULL << i;
    }
    return bb;
}
};  // namespace BitBoard

namespace dirs {
// Directions. Each direction changes an idx by the corresponding amount.
static constexpr int N = 8;
static constexpr int E = 1;
static constexpr int S = -8;
static constexpr int W = -1;
static constexpr int SW = S + W;
static constexpr int NE = N + E;
static constexpr int SE = S + E;
static constexpr int NW = N + W;
}  // namespace dirs

namespace masks {  // Containing masks for bitboards. E.g. mask out top row or someth(ing similar.
static constexpr uint64_t bottom = 0b11111111;
static constexpr uint64_t top = BitBoard::shift_bb(bottom, dirs::N, 7);
static constexpr uint64_t left = 0x0101010101010101;
static constexpr uint64_t right = BitBoard::shift_bb(left, dirs::E, 7);
static constexpr uint64_t sides = left | right;
static constexpr uint64_t fill = 0xFFFFFFFFFFFFFFFF;
static constexpr uint64_t around = sides | top | bottom;
[[nodiscard("The return value should be handled as a uint64_t.")]]
static inline constexpr uint64_t row(uint8_t r) {
    return bottom << 8 * r;
}  // mask for row.
[[nodiscard("The return value should be handled as a uint64_t.")]]
static inline constexpr uint64_t col(uint8_t c) {
    return left << c;
}  // mask for col.

static constexpr uint64_t edge_mask(int dir) {
    uint64_t outmask = 0;
    switch (dir) {
    case dirs::E:
        outmask = right;
        break;
    case dirs::W:
        outmask = left;
        break;
    case dirs::S:
        outmask = bottom;
        break;
    case dirs::N:
        outmask = top;
        break;
    case dirs::NE:
        outmask = top | right;
        break;
    case dirs::NW:
        outmask = top | left;
        break;
    case dirs::SE:
        outmask = bottom | right;
        break;
    case dirs::SW:
        outmask = bottom | left;
        break;
    default:
        std::cerr << "err: wrong dir" << std::endl;
        abort();
        break;
    }
    return outmask;
}
}  // namespace masks

#endif
