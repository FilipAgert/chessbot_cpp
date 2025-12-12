// Copyright 2025 Filip Agert
#ifndef TABLES_H
#define TABLES_H

#include <algorithm>
#include <bitboard.h>
#include <board.h>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <piece.h>
#include <random>
#include <vector>
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
    uint64_t hash = 0;
    Move bestmove = Move();
    int eval = 0;  // Integrated bounds and values. 4n = exact eval n. 4n + 1 = a lower bound. 4n -
                   // 1 = an upper bound.
    uint8_t nodetype = invalid;
    uint8_t depth = 0;  // to what depth was this move searched? Can only accept if our depth is
                        // same or shallower.
    inline bool is_valid_move() {
        return bestmove.source != bestmove.target && bestmove.source != err_val8;
    }

    bool is_exact() { return nodetype == exact; }
    bool is_lb() { return nodetype == lb; }
    bool is_ub() { return nodetype == ub; }
    enum nodetype { exact, lb, ub, invalid };
};
constexpr transposition_entry nullentry = {0, Move(0, 0), 0, transposition_entry::invalid,
                                           0};  // transposition_entry{0, Move(0, 0), 0, 4, 0};

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
    template <Piece_t p, bool is_white> void hash_piece(uint64_t &hash, const Board &board);
    template <Piece_t p> void hash_both_piece(uint64_t &hash, const Board &board);
    template <Piece_t p, bool is_white> constexpr static uint8_t piece_key() {
        uint8_t col_offset = is_white ? 0 : 6;
        return col_offset + (p - 1);
    }
    void hash_ep(uint64_t &hash, const Board &board);
    void hash_castle(uint64_t &hash, const Board &board) {
        hash ^= castle_numbers[board.get_castling()];
    }
    void hash_turn(uint64_t &hash, const Board &board) {
        if (board.get_turn_color() == pieces::black) {
            hash ^= black_number;
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
struct transposition_table {
    static constexpr size_t entry_size = sizeof(transposition_entry);
    static constexpr int size_MB = 16;
    static constexpr int nbits = [] constexpr {
        constexpr size_t num_entries = size_MB * 1000000 / entry_size;
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
    static constexpr uint64_t mask = (1ULL << nbits) - 1;  // lowest nbits set high.
    static constexpr int actual_size_kB = (table_size * entry_size) / 1000;
    std::array<transposition_entry, table_size> arr;  // array holding the data.
    size_t hits = 0;        // how many times did we access the table and find the board inside?
    size_t misses = 0;      // how many times did we access the table and not find the board?
    size_t collisions = 0;  // how many times did we have a hash collision?
    size_t overwrites = 0;
    size_t writes = 0;

    /**
     * @brief Gets key to access the table with
     *
     * @param[in] hash hash of board
     * @return key to access array with.
     */
    static inline constexpr size_t get_key(uint64_t hash) { return hash & mask; }
    /**
     * @brief Gets entry from table. If hash matches, return entry.
     *
     * @param[in] hash hash of board.
     */
    std::optional<transposition_entry> get(uint64_t hash);
    inline void set(transposition_entry entry) {
        size_t key = get_key(entry.hash);
        writes++;
        if (arr[key].nodetype != transposition_entry::invalid) {
            overwrites++;
        }
        arr[key] = entry;
    }
    inline void store(uint64_t hash, Move bestmove, int eval, uint8_t nodetype, uint8_t depth) {
        assert(bestmove.source != bestmove.target);
        assert(bestmove.source != err_val8);
        set({hash, bestmove, eval, nodetype, depth});
    }
    /**
     * @brief Gets if the entry provided is
     *
     * @param[in] entry entry to validate
     * @param[in] depth depth this state occured at
     * @return True if the current depth is smaller than or equal the entries depth.
     */
    static bool is_useable_entry(const transposition_entry entry, const int depth) {
        return depth <= entry.depth;
    }
    void clear() {
        std::fill(arr.begin(), arr.end(),
                  transposition_entry{0, Move(), 0, transposition_entry::invalid, 0});
        writes = 0;
        overwrites = 0;
    }

    /**
     * @brief Gets the load factor of the hash table in permille. This is the ratio of filled slots.
     * entries.
     *
     * @return [Load factor of table in permille]
     */
    int load_factor() const;

    std::vector<Move> get_pv(Board &board, int depth) {
        std::vector<Move> pv_line;
        for (int i = 0; i <= depth; i++) {
            uint64_t hash = ZobroistHasher::get().hash_board(board);
            std::optional<transposition_entry> entry = get(hash);
            if (entry) {
                if (entry.value().is_valid_move()) {
                    pv_line.push_back(entry.value().bestmove);
                    board.do_move(entry.value().bestmove);
                }
            }
        }
        for (int i = pv_line.size() - 1; i >= 0; i--) {
            board.undo_move(pv_line[i]);
        }
        return pv_line;
    }
};
#endif
