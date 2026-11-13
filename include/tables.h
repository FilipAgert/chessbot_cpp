// Copyright 2025 Filip Agert
#ifndef TABLES_H
#define TABLES_H

#include "integer_representation.h"
#include <algorithm>
#include <bitboard.h>
#include <board.h>
#include <cassert>
#include <cmath>
#include <config.h>
#include <cstdint>
#include <eval.h>
#include <piece.h>
#include <random>
#include <stack>
#include <vector>
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
    inline int count_elem(HashEntry hash) const {
        int ctr = 0;
        int num_pieces = hash.num_pieces;
        for (int i = std::min(top_idx, max_size - 1); i >= 0; i--) {
            if (stack[i].num_pieces < num_pieces)
                break;
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
    inline int atleast_num(HashEntry hash, int num) {
        int ctr = 0;
        int num_pieces = hash.num_pieces;
        for (int i = std::min(top_idx, max_size - 1); i >= 0; i--) {
            if (stack[i].num_pieces < num_pieces)
                break;
            ctr += stack[i] == hash;
            if (ctr >= num)
                return true;
        }
        return false;
    }
};

class ZobroistHasher {
 private:
    ZobroistHasher() {
        initialize_engine();
        randomize_local_vars();
    }

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

 public:
    static ZobroistHasher &get() {
        static ZobroistHasher instance;
        return instance;
    }

    std::array<std::array<uint64_t, 64>, 12> piece_numbers;  // one number for each piece and square
    std::array<uint64_t, 16> castle_numbers;                 // one for each castle combination
    std::array<uint64_t, 8> ep_numbers;                      // file of ep
    uint64_t black_number;                                   // indicate if black is playing or not.
    /**
     * @brief Adds to hash for a piece
     *
     * @tparam Piece_t type of piece
     * @tparam iswhite color of piece
     * @param[inout] hash Hash as input. new hash as output
     * @param[in] board to hash.
     */
    template <Piece_t p, bool is_white> void hash_piece(uint64_t &hash, const Board &board) const;
    template <Piece_t p> void hash_both_piece(uint64_t &hash, const Board &board) const;
    template <Piece_t p, bool is_white> constexpr static uint8_t piece_key() {
        uint8_t col_offset = is_white ? 0 : 6;
        return col_offset + (p - 1);
    }
    void hash_ep(uint64_t &hash, const Board &board) const;
    void hash_castle(uint64_t &hash, const Board &board) const;
    void hash_turn(uint64_t &hash, const Board &board) const;

    uint64_t rand_uint64_t();
    /**
     * @brief Generate hash from a board
     *
     * @param[in] board Board to hash
     * @return hash of board.
     */
    uint64_t hash_board(const Board &board) const;
    uint64_t delta(const HashDelta &delta) const;
    /**
     * @brief Hash can be incremented by XORing the parts that changed. This is color and pieces
     * moved and castle information.
     *
     * @tparam forward true if move is forwards in time, false if backwards (undo).
     * @param[in] hash hash updated with the move.
     */
    template <bool forward> void update_hash(uint64_t &hash, const Move move) {}
};
struct TranspositionEntry {
    static constexpr int keep_hashbits = 48;
    static constexpr int shift_hash = 64 - 48;
    uint64_t hash : keep_hashbits = 0;
    // Why 48? because the key contains part of the hash anyway. 64-48 = 16 bits minimum in key for this to not destroy information.
    // Key uses rightmost bits, so
    // this must use the leftmost bits.
    uint8_t nodetype = invalid;
    uint8_t depth = 0;  // to what depth was this move searched? Can only accept if our depth is
    int eval = 0;       // Integrated bounds and values. 4n = exact eval n. 4n + 1 = a lower bound. 4n -
    Move bestmove = Move();
    uint16_t age = 0;  // what age
    // 1 = an upper bound.
    // same or shallower.
    inline bool is_valid_move() const { return (bestmove.source != bestmove.target); }

    bool is_exact() const { return nodetype == exact; }
    bool is_lb() const { return nodetype == lb; }
    bool is_ub() const { return nodetype == ub; }
};
constexpr TranspositionEntry nullentry = {0, invalid, 0, 0, Move(0, 0), 0};  // transposition_entry{0, Move(0, 0), 0, 4, 0};

struct TranspositionBucket {
    TranspositionEntry depth_pref;
    TranspositionEntry always_replace;
};

struct TranspositionTable {
 private:
    std::vector<TranspositionBucket> arr;

 public:
    static constexpr size_t entry_size = sizeof(TranspositionBucket);
    uint64_t mask;
    size_t hits = 0;        // how many times did we access the table and find the board inside?
    size_t misses = 0;      // how many times did we access the table and not find the board?
    size_t collisions = 0;  // how many times did we have a hash collision?
    size_t overwrites = 0;
    size_t writes = 0;
    uint16_t age = 0;  // what is the age of this table?
    int table_size_MB = 0;
    /**
     * @brief Initializes the table to a desired size in MB.
     *
     * @param desired_size_MB
     */
    TranspositionTable(int desired_size_MB);

    /**
     * @brief Gets key to access the table with
     *
     * @param[in] hash hash of board
     * @return key to access array with.
     */
    inline size_t get_key(uint64_t hash) { return hash & mask; }
    /**
     * @brief Gets entry from table. If hash matches, return entry.
     *
     * @param[in] hash hash of board.
     */
    std::optional<TranspositionEntry> get(uint64_t hash);

    void store(uint64_t hash, Move bestmove, int eval, uint8_t nodetype, uint8_t depth);
    /**
     * @brief Gets if the entry provided is
     *
     * @param[in] entry entry to validate
     * @param[in] depth depth this state occured at
     * @return True if the current depth is smaller than or equal the entries depth.
     */
    bool is_useable_entry(const TranspositionEntry entry, const int depth) {
        return (static_cast<int>(age) + depth) <= entry.depth + static_cast<int>(entry.age);
    }
    /**
     * @brief Clears the table.
     *
     */
    void clear();

    /**
     * @brief Gets the load factor of the hash table in permille. This is the ratio of filled slots.
     * entries.
     *
     * @return [Load factor of table in permille]
     */
    int load_factor() const;

    /**
     * @brief Gets the principal variation for as long as the table has it.
     *
     * @param board
     * @param depth
     * @return std::vector<Move>
     */
    std::vector<Move> get_pv(Board &board, int depth, StateStack &stack);

    void increment_age();
    void reset_age();
};
#endif
