// Copyright 2025 Filip Agert
#ifndef TABLES_H
#define TABLES_H

#include <algorithm>
#include <bitboard.h>
#include <board.h>
#include <cmath>
#include <cstdint>
#include <piece.h>
#include <random>
/**
 * @brief Class for storing the occured game states for checking 3 move repetion draws.
 */

class StateStack {
 private:
    int top_idx = -1;
    constexpr static int max_size = 512;
    std::array<uint64_t, max_size> stack;

 public:
    void print() {
        for (int i = 0; i <= top_idx; i++) {
            BitBoard::print(stack[i]);
        }
    }
    inline void push(uint64_t hash) { stack[++top_idx] = hash; }
    inline void pop() { top_idx--; }
    inline uint64_t top() { return stack[top_idx]; }
    /**
     * @brief Counts number of occurences of hash in stack.
     *
     * @param[[TODO:direction]] hash [TODO:description]
     * @return [TODO:description]
     */
    inline int count_elem(uint64_t hash) {
        int ctr = 0;
        for (int i = std::min(top_idx, max_size - 1); i >= 0; i--) {
            ctr += stack[i] == hash;
        }
        return ctr;
    }

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
    inline bool atleast_num(uint64_t hash, int num) {
        int ctr = 0;
        int idx = std::min(top_idx, max_size - 1);
        while ((ctr < num) && (idx >= 0)) {
            ctr += hash == stack[idx];
            idx--;
        }
        return ctr == num;
    }
};

struct transposition_entry {
    uint64_t hash;
    Move bestmove;
    int IBV;  // Integrated bounds and values. 4n = exact eval n. 4n + 1 = a lower bound. 4n - 1 =
              // an upper bound.
    int depth;
    int age;  // Age of node.

    bool get_eval() { return IBV / 4; }
    bool is_exact() { return (IBV & 0b111) == 0b100; }
    bool is_ub() { return (IBV & 0b111) == 0b011; }
    bool is_lb() { return (IBV & 0b111) == 0b101; }
    int IBV_LB(int lb) { return lb * 4 + 1; }
    int IBV_exact(int eval) { return eval * 4; }
    int IBV_UB(int ub) { return ub * 4 - 1; }
    static constexpr int entry_size = 32;  // bytes
};
struct transposition_table {
    static constexpr int size_MB = 16;
    static constexpr int nbits = [] constexpr {
        constexpr int num_entries = size_MB * 1000000 / transposition_entry::entry_size;
        int msb = 0;
        int val = num_entries;
        while (val >>= 1)
            msb++;

        int numbits;
        if (num_entries & ~(1ULL << msb)) {  // if other bits were set high too.
            numbits = msb + 1;
        } else {
            numbits = msb;
        }

        return numbits;
    }();
    static constexpr int table_size = 1ULL << nbits;
    static constexpr int actual_size_kB = (table_size * transposition_entry::entry_size) / 1000;
};

class ZobroistHasher {
 private:
    ZobroistHasher() {
        initialize_engine();
        randomize_local_vars();
    }

 public:
    static ZobroistHasher &get() {
        static ZobroistHasher instance;
        return instance;
    }

    std::array<std::array<uint64_t, 64>, 12> piece_numbers;  // one number for each piece and square
    std::array<uint64_t, 16> castle_numbers;                 // one for each castle combination
    std::array<uint64_t, 8> ep_numbers;                      // file of ep
    uint64_t black_number;                                   // indicate if black is playing or not.
    std::uniform_int_distribution<uint64_t> generator;
    std::mt19937_64 engine;
    /**
     * @brief Generates a random uint64_t number. Seeded at startup so will be different each
     * session.
     *
     * @return random uint64_t
     */
    void initialize_engine();
    void randomize_local_vars();

    /**
     * @brief Adds to hash for a piece
     *
     * @tparam Piece_t type of piece
     * @tparam iswhite color of piece
     * @param[inout] hash Hash as input. new hash as output
     * @param[in] board to hash.
     */
    template <Piece_t p, bool is_white> void hash_piece(uint64_t &hash, const Board &board);
    template <Piece_t p> void hash_both_piece(uint64_t &hash, const Board &board);
    template <Piece_t p, bool is_white> constexpr static uint8_t piece_key() {
        uint8_t col_offset = is_white ? 0 : 6;
        return col_offset + (p - 1);
    }
    void hash_ep(uint64_t &hash, const Board &board);
    void hash_castle(uint64_t &hash, const Board &board) { hash ^= board.get_castling(); }
    void hash_turn(uint64_t &hash, const Board &board) {
        if (board.get_turn_color() == pieces::black) {
            hash ^= ZobroistHasher::black_number;
        }
    }

    uint64_t rand_uint64_t();
    /**
     * @brief Generate hash from a board
     *
     * @param[in] board Board to hash
     * @return hash of board.
     */
    uint64_t hash_board(const Board &board);
    /**
     * @brief Hash can be incremented by XORing the parts that changed. This is color and pieces
     * moved and castle information.
     *
     * @tparam forward true if move is forwards in time, false if backwards (undo).
     * @param[in] hash hash updated with the move.
     */
    template <bool forward> void update_hash(uint64_t &hash, const Move move) {}
};
#endif
