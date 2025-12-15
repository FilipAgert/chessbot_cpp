// Copyright 2025 Filip Agert
#ifndef BITBOARD_H
#define BITBOARD_H
#include <integer_representation.h>

#include <array>
#include <bitset>
#include <cstdint>
#include <iostream>
#include <notation_interface.h>
#include <string>
namespace BitBoard {
#define BitLoop(X) for (; X; X = X & (X - 1))  // The bitloop iterates over a bitboard, repeatedly clearing the LSB.
#define BB uint64_t

/** \
 * @brief Shifts the bitboard in a certain direction n # of steps. Branchless execution \
 *                                                                                                 \
 * @param board board to shift \
 * @param dir direction to move in (N,E,S,W etc) \
 * @param steps How many steps to move in this direction \
 * @return shifted bitboard \
 */
[[nodiscard("The return value should be handled as a uint64_t.")]]
static inline constexpr uint64_t shift_bb(const uint64_t board, const int dir, const uint8_t steps) {
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

inline constexpr uint64_t clear_lsb(uint64_t bb) { return bb & (bb - 1); }
/**
 * @brief For each high bit in the input bitboard, clear it and repeatedly OR the outputs of an
 * input function. return result
 *
 * @param[inout] bb input bb to call function with idx of high bits
 * @param[in] occ occupation bitboard
 * @return orsum_i  f_i(idx_i, occ)
 */
template <class T, class Func> uint64_t bitboard_operate_or(uint64_t &bb, T arg, Func func) {
    uint64_t out = 0;
    BitLoop(bb) { out |= func(lsb(bb), arg); }
    return out;
}
/**
 * @brief For each high bit in the input bitboard, clear it and repeatedly OR the outputs of an
 * input function. return result
 *
 * @param[inout] bb input bb to call function with idx of high bits. Destroys it.
 * @return orsum_i  f_i(idx_i, occ)
 */
template <class Func> uint64_t bitboard_operate_or(uint64_t &bb, Func func) {
    uint64_t out = 0;
    BitLoop(bb) { out |= func(lsb(bb)); }
    return out;
}

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
/*
 * @brief Shoots ray from origin, up to edge of board and returns the corresponding bitboard.
 *
 * @param origin
 * @param dir
 * @return uint64_t
 */
constexpr uint64_t ray(const uint64_t origin, const int dir, const uint64_t blocker_bb, const int steps) {
    // How to ensure no wrap-around?
    uint64_t hit = origin;
    uint64_t mask = edge_mask(dir) | blocker_bb;  // The edge mask ensures we do not wrap-around.The blocker
                                                  // mask ensures that we do not keep going through somebody.
    hit |= BitBoard::shift_bb(~edge_mask(dir) & hit,
                              dir);  // Take first step without the blocker bb, since the blocker bb includes self.
    for (int i = 2; i <= steps; i++) {
        hit |= BitBoard::shift_bb((~mask) & hit,
                                  dir);  // Shift the mask in dir direction, but only on non-masked places.
    }
    return hit & ~origin;  // Exclude origin, since the piece does not attack itself.
}
constexpr uint64_t ray(const uint64_t origin, const int dir, const uint64_t blocker_bb) { return ray(origin, dir, blocker_bb, 7); }
constexpr uint64_t ray(const uint64_t origin, const int dir) { return ray(origin, dir, 0, 7); }
/**
 * @brief Mask of squares in between from (exclusive) to (inclusive). If rect_lookup[i][i] return 0.
 *
 * @param[in] from from square. not included in mask
 * @param[in] to to square. included in mask.
 */
static constexpr std::array<std::array<BB, 64>, 64> rect_lookup = [] {
    std::array<std::array<BB, 64>, 64> arr;
    arr.fill({0ULL});
    for (int i = 0; i < 64; i++) {
        // Vertical:
        int rowinit = NotationInterface::row(i);
        int colinit = NotationInterface::col(i);
        BB startsq = BitBoard::one_high(i);
        for (int r = 0; r < 8; r++) {
            int j = NotationInterface::idx(r, colinit);
            BB to = BitBoard::one_high(j);
            if (i == j)
                continue;
            // Fill all between i and j.
            // If positive, move up.
            int dir = j > i ? dirs::N : dirs::S;
            BB mask = ray(startsq, dir, to);
            arr[i][j] = mask & ~startsq;
        }

        for (int c = 0; c < 8; c++) {
            int j = NotationInterface::idx(rowinit, c);
            BB to = BitBoard::one_high(j);
            // Fill all between i and j.
            // If positive, move up.
            if (i == j)
                continue;

            int dir = c > colinit ? dirs::E : dirs::W;
            BB mask = ray(startsq, dir, to);
            arr[i][j] = mask & ~startsq;
        }

        {
            // NE/SW diagonal.
            // If row > col : COL = 0.
            int c_start = (rowinit >= colinit) ? 0 : colinit - rowinit;
            int r_start = (rowinit >= colinit) ? rowinit - colinit : 0;

            for (int r = r_start, c = c_start; r < 8 && c < 8; r++, c++) {
                int j = NotationInterface::idx(r, c);

                if (i == j)
                    continue;

                BB to = BitBoard::one_high(j);

                int dir = j > i ? dirs::NE : dirs::SW;

                BB mask = ray(startsq, dir, to);
                arr[i][j] = mask & ~startsq;
            }
        }

        {
            // NW/SE diagonal.
            // row + column = constant.
            int c_start = (rowinit + colinit < 7) ? colinit + rowinit : 7;
            int r_start = (rowinit + colinit < 7) ? 0 : rowinit + colinit - 7;

            for (int r = r_start, c = c_start; r < 8 && c >= 0; r++, c--) {
                int j = NotationInterface::idx(r, c);

                if (i == j)
                    continue;

                BB to = BitBoard::one_high(j);

                int dir = j > i ? dirs::NW : dirs::SE;

                BB mask = ray(startsq, dir, to);
                arr[i][j] = mask & ~startsq;
            }
        }
    }
    return arr;
}();
}  // namespace masks
#endif
