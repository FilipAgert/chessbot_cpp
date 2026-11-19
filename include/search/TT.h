// Copyright 2025 Filip Agert
#ifndef TABLES_H
#define TABLES_H

#include "core/board.h"
#include "core/integer_representation.h"
#include "core/zobrist.h"
#include "search/state_history.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

struct TranspositionEntry {
    static constexpr int keep_hashbits = 48;
    static constexpr int shift_hash = 64 - keep_hashbits;
    uint64_t hash : keep_hashbits = 0;
    // Why 48? because the key contains part of the hash anyway. 64-48 = 16 bits minimum in key for this to not destroy information.
    // Key uses rightmost bits, so
    // this must use the leftmost bits.
    SearchNodeType nodetype : 2 = SearchNodeType::invalid;
    uint8_t depth = 0; // to what depth was this move searched? Can only accept if our depth is
    int eval = 0;      // Integrated bounds and values. 4n = exact eval n. 4n + 1 = a lower bound. 4n -
    Move bestmove = Move();
    uint16_t age = 0; // what age
    // 1 = an upper bound.
    // same or shallower.
    inline bool is_valid_move() const { return (bestmove.source != bestmove.target); }

    bool is_exact() const { return nodetype == SearchNodeType::exact; }
    bool is_lb() const { return nodetype == SearchNodeType::lb; }
    bool is_ub() const { return nodetype == SearchNodeType::ub; }
};
constexpr TranspositionEntry nullentry = {0, SearchNodeType::invalid, 0, 0, Move(0, 0), 0}; // transposition_entry{0, Move(0, 0), 0, 4, 0};

struct alignas(32) TranspositionBucket {
    TranspositionEntry depth_pref;
    TranspositionEntry always_replace;
};
static_assert(sizeof(TranspositionBucket) == 32);
static_assert(alignof(TranspositionBucket) == 32);
/**
 * @brief Return type for probing TT.
 *
 */
struct TTProbeResult {
    int score = 0;
    std::optional<Move> tt_move = std::nullopt;
    SearchNodeType bound = SearchNodeType::invalid;
    bool cutoff = false;
};
struct TranspositionTable {
  private:
    TranspositionBucket *arr = nullptr;
    size_t arr_bytes = 0;
    size_t table_size = 0;
    const ZobroistHasher &hasher;

  public:
    static constexpr size_t entry_size = sizeof(TranspositionBucket);
    uint64_t mask;
    size_t hits = 0;       // how many times did we access the table and find the board inside?
    size_t misses = 0;     // how many times did we access the table and not find the board?
    size_t collisions = 0; // how many times did we have a hash collision?
    size_t overwrites = 0;
    size_t writes = 0;
    int table_size_MB = 0;

    uint16_t age = 0;
    /**
     * @brief Initializes the table to a desired size in MB.
     *
     * @param desired_size_MB
     */
    TranspositionTable(const ZobroistHasher &hasher, int desired_size_MB);
    TranspositionTable(const TranspositionTable &) = delete;
    TranspositionTable &operator=(const TranspositionTable &) = delete;
    ~TranspositionTable();

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

    void store(uint64_t hash, Move bestmove, int eval, SearchNodeType nodetype, uint8_t depth);
    /**
     * @brief Gets if the entry provided is
     *
     * @param[in] entry entry to validate
     * @param[in] depth depth this state occured at
     * @return True if the current depth is smaller than or equal the entries depth.
     */
    bool is_useable_entry(const TranspositionEntry entry, const int depth) { return (depth) <= entry.depth; }
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

    /**
     * @brief Fetches the hash from cache. to be done immediately after zobroisthasher hashes on do_move.
     * 0: read 1: low temporal locality.
     * @param hash
     */
    inline void prefetch(uint64_t hash) { __builtin_prefetch(&arr[get_key(hash)], 0, 1); }

    void increment_age();
    void reset_age();
};
#endif
