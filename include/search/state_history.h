#pragma once
#include "core/bitboard.h"
#include <cassert>
#include <cmath>
#include <cstdint>
/**
 * @brief Class for storing the occured game states for checking 3 move repetion draws.
 */
struct HashEntry {
    uint64_t hash;
    uint8_t num_pieces;

    inline bool operator==(const HashEntry &other) const { return hash == other.hash; }
    inline bool operator==(uint64_t hsh) const { return hash == hsh; }
    inline HashEntry operator^(uint64_t other_hash) const { return HashEntry{hash ^ other_hash, num_pieces}; }
};
class StateStack {
  private:
    int top_idx = -1;
    constexpr static int max_size = 512;
    std::array<HashEntry, max_size> stack;

  public:
    inline int len() { return top_idx + 1; }
    void print() {
        for (int i = 0; i <= top_idx; i++) {
            BitBoard::print(stack[i].hash);
        }
    }
    inline void push(HashEntry hash) { stack[++top_idx] = hash; }
    inline void pop() { top_idx--; }
    inline HashEntry top() { return stack[top_idx]; }
    /**
     * @brief Counts number of occurences of hash in stack.
     *
     * @param[[TODO:direction]] hash [TODO:description]
     * @return [TODO:description]
     */
    int count_elem(HashEntry hash) const;
    bool is_repetition(int root_idx) const;

    /**
     * @brief Resets stack.
     *
     */
    inline void reset() { top_idx = -1; }

    /**
     * @brief Checks if there are atleast some number of occurences or more in the stack
     *
     * @param[in] hash hash to check for
     * @param[in] num number of hashes in stack
     * @return true if there are at least num hashes of this hash in the stack.
     */
    int atleast_num(HashEntry hash, int num);
};
