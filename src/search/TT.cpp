// Copyright 2025 Filip Agert
#include "search/TT.h"
#include "core/board.h"
#include "eval/eval.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <stack>
#include <stdexcept>
#include <sys/mman.h>
TranspositionTable::TranspositionTable(const ZobroistHasher &hasher, int desired_size_MB) : hasher(hasher) {
    constexpr size_t HUGE_PAGE = size_t(2) << 20; // 2 MB

    size_t num_entries = size_t(desired_size_MB) * 1024 * 1024 / entry_size;

    // round up to the next power of two
    int numbits = 0;
    while ((size_t(1) << numbits) < num_entries)
        numbits++;

    if (!(numbits > TranspositionEntry::shift_hash))
        throw std::runtime_error("must have more bits in table than in the hash shift");

    table_size = size_t(1) << numbits;
    mask = table_size - 1; // lowest nbits set high.

    // bytes, rounded up to a multiple of 2 MB (aligned_alloc requires this)
    arr_bytes = (table_size * entry_size + HUGE_PAGE - 1) & ~(HUGE_PAGE - 1);

    arr = static_cast<TranspositionBucket *>(std::aligned_alloc(HUGE_PAGE, arr_bytes));
    if (!arr)
        throw std::bad_alloc();

    if (madvise(arr, arr_bytes, MADV_HUGEPAGE) != 0)
        perror("madvise");

    table_size_MB = table_size * entry_size / (1024 * 1024);
    clear(); // memset touches every page
}

std::optional<TranspositionEntry> TranspositionTable::get(uint64_t hash) {
    size_t key = get_key(hash);
    TranspositionBucket &curr = arr[key];
    std::optional<TranspositionEntry> maybe;
    uint64_t shifted = hash >> TranspositionEntry::shift_hash;
    if (curr.depth_pref.hash == shifted && curr.depth_pref.nodetype != SearchNodeType::invalid) {
        hits++;
        maybe = std::make_optional(curr.depth_pref);
    } else if (curr.always_replace.hash == shifted && curr.always_replace.nodetype != SearchNodeType::invalid) {
        hits++;
        maybe = std::make_optional(curr.always_replace);
    } else {
        maybe = {};
        misses++;
        if ((curr.depth_pref.hash | curr.always_replace.hash) > 0) {
            collisions++;
        }
    }
    return maybe;
}
std::vector<Move> TranspositionTable::get_pv(Board &board, int depth, StateStack &stack) {
    std::vector<Move> pv_line;
    std::stack<RestoreMoveInfo> restore_stack;
    int pre_len = stack.len();

    for (int i = 0; i <= depth; i++) {
        uint64_t hash = hasher.hash_board(board);
        std::optional<TranspositionEntry> entry = get(hash);
        if (!entry || !entry->is_valid_move())
            break;

        const bool white = board.get_turn_color() == pieces::white;
        const Move m = entry->bestmove;
        bool is_legal = board.is_move_legal(m);
        if (!is_legal)
            break; // stale entry or hash collision: stop the PV here

        RestoreMoveInfo info = white ? board.do_move<true>(m) : board.do_move<false>(m);
        pv_line.push_back(m);
        restore_stack.push(info);
        stack.push({hasher.hash_board(board), board.get_num_pieces()});

        if (Eval::forced_draw_ply(board) || stack.atleast_num(stack.top(), 2))
            break;
    }

    for (int i = (int)pv_line.size() - 1; i >= 0; i--) {
        RestoreMoveInfo info = restore_stack.top();
        restore_stack.pop();
        stack.pop();
        board.undo_move(info, pv_line[i]);
    }
    assert(stack.len() == pre_len);
    return pv_line;
}
int TranspositionTable::load_factor() const { return writes > 0 ? (overwrites * 1000) / writes : 0; }

void TranspositionTable::clear() {
    std::memset(arr, 0, arr_bytes);
    // std::fill(arr.begin(), arr.end(),
    //           TranspositionBucket{TranspositionEntry{0, SearchNodeType::invalid, 0, 0, Move()}, TranspositionEntry{0, SearchNodeType::invalid, 0, 0,
    //           Move()}});
    writes = 0;
    overwrites = 0;
    hits = 0;       // how many times did we access the table and find the board inside?
    misses = 0;     // how many times did we access the table and not find the board?
    collisions = 0; // how many times did we have a hash collision?
    age = 0;
}

void TranspositionTable::store(uint64_t hash, Move bestmove, int eval, SearchNodeType nodetype, uint8_t depth) {
    uint64_t h = (hash >> TranspositionEntry::shift_hash);
    size_t key = get_key(hash);
    writes++;
    TranspositionBucket &bucket = arr[key];
    TranspositionEntry &dp = bucket.depth_pref;
    TranspositionEntry &ar = bucket.always_replace;
    bool dp_valid = dp.nodetype != SearchNodeType::invalid;
    bool ar_valid = ar.nodetype != SearchNodeType::invalid;
    // 1. Fill up buckets (Do not if same hash on both)
    // cases:
    // depth empty,  2 empty
    // depth full,   2 empty
    // depth full,   2 full

    TranspositionEntry ne{h, nodetype, depth, eval, bestmove, age};
    // benchmark never demoting.
    if (dp_valid && dp.hash == h) {
        if (depth >= dp.depth) {
            if (!ne.is_valid_move())
                ne.bestmove = dp.bestmove;
            dp = ne;
        }
        return; // deeper depth already exist for this state.
    }

    if (!dp_valid || (static_cast<uint16_t>(age) + depth) >= (static_cast<uint16_t>(dp.depth) + dp.age)) { // different board states => age matters.
        if (dp_valid) {
            if (ar_valid && ar.hash != dp.hash)
                overwrites++;
            ar = dp;
            // move into always replace slot.
        } else if (ar_valid && ar.hash == h) {
            ar.nodetype = SearchNodeType::invalid;
            // invalidate.
        }

        dp = ne;
    } else {
        if (ar_valid && ar.hash != h)
            overwrites++;
        ar = ne;
    }
}
void TranspositionTable::increment_age() { age++; }
void TranspositionTable::reset_age() { age = 0; }

TranspositionTable::~TranspositionTable() { std::free(arr); }