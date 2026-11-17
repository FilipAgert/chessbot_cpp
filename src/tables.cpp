// Copyright 2025 Filip Agert
#include "config.h"
#include <bitboard.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <new>
#include <piece.h>
#include <random>
#include <stdexcept>
#include <sys/mman.h>
#include <tables.h>
void ZobroistHasher::initialize_engine() {
    std::random_device sd;
    engine.seed(sd());
}
uint64_t ZobroistHasher::rand_uint64_t() { return generator(engine); }
void ZobroistHasher::randomize_local_vars() {
    for (int p = 0; p < 12; p++) {
        for (int sq = 0; sq < 64; sq++) {
            piece_numbers[p][sq] = rand_uint64_t();
        }
    }
    for (int i = 0; i < 16; i++) {
        castle_numbers[i] = rand_uint64_t();
    }
    for (int i = 0; i < 8; i++) {
        ep_numbers[i] = rand_uint64_t();
    }
    black_number = rand_uint64_t();
}
template <Piece_t p, bool is_white> void ZobroistHasher::hash_piece(uint64_t &hash, const Board &board) const {
    BB piece_bb = board.get_piece_bb<p, is_white>();
    constexpr uint8_t key = piece_key<p, is_white>();
    BitLoop(piece_bb) {
        uint8_t LSB = BitBoard::lsb(piece_bb);
        uint64_t rand = piece_numbers[key][LSB];
        hash ^= rand;
    }
}
template <Piece_t p> void ZobroistHasher::hash_both_piece(uint64_t &hash, const Board &board) const {
    hash_piece<p, true>(hash, board);
    hash_piece<p, false>(hash, board);
}
void ZobroistHasher::hash_ep(uint64_t &hash, const Board &board) const {
    if (board.get_en_passant()) {
        uint8_t ep_file = NotationInterface::col(board.get_en_passant_square());
        hash ^= ep_numbers[ep_file];
    }
}
uint64_t ZobroistHasher::hash_board(const Board &board) const {
    uint64_t hash = 0;
    hash_both_piece<pieces::king>(hash, board);
    hash_both_piece<pieces::queen>(hash, board);
    hash_both_piece<pieces::rook>(hash, board);
    hash_both_piece<pieces::bishop>(hash, board);
    hash_both_piece<pieces::knight>(hash, board);
    hash_both_piece<pieces::pawn>(hash, board);
    hash_ep(hash, board);
    hash_castle(hash, board);
    hash_turn(hash, board);
    return hash;
};
// Maps a full piece value (color | type) to the index used in piece_numbers.
static constexpr uint8_t key_of(uint8_t piece) {
    if (piece == pieces::none)
        return 0;
    uint8_t col_offset = ((piece & pieces::color_mask) == pieces::black) ? 6 : 0;
    return col_offset + (piece & pieces::piece_mask) - 1;
}
uint64_t ZobroistHasher::delta(const HashDelta &d) const {
    uint64_t h = 0;
    h = black_number;  // turn flips every move
    if (d.moved_piece)
        h ^= piece_numbers[key_of(d.moved_piece)][d.from];
    if (d.placed_piece)
        h ^= piece_numbers[key_of(d.placed_piece)][d.to];
    if (d.captured_piece)
        h ^= piece_numbers[key_of(d.captured_piece)][d.capture_sq];
    if (d.rook_piece) {
        uint8_t rk = key_of(d.rook_piece);
        h ^= piece_numbers[rk][d.rook_from] ^ piece_numbers[rk][d.rook_to];
    }
    h ^= castle_numbers[d.old_castle] ^ castle_numbers[d.new_castle];
    if (d.old_ep)
        h ^= ep_numbers[NotationInterface::col(d.old_ep)];
    if (d.new_ep)
        h ^= ep_numbers[NotationInterface::col(d.new_ep)];
    return h;
}
void ZobroistHasher::hash_castle(uint64_t &hash, const Board &board) const { hash ^= castle_numbers[board.get_castling()]; }
void ZobroistHasher::hash_turn(uint64_t &hash, const Board &board) const {
    if (board.get_turn_color() == pieces::black) {
        hash ^= black_number;
    }
}
TranspositionTable::TranspositionTable(int desired_size_MB) {
    constexpr size_t HUGE_PAGE = size_t(2) << 20;  // 2 MB

    size_t num_entries = size_t(desired_size_MB) * 1024 * 1024 / entry_size;

    // round up to the next power of two
    int numbits = 0;
    while ((size_t(1) << numbits) < num_entries)
        numbits++;

    if (!(numbits > TranspositionEntry::shift_hash))
        throw std::runtime_error("must have more bits in table than in the hash shift");

    table_size = size_t(1) << numbits;
    mask = table_size - 1;  // lowest nbits set high.

    // bytes, rounded up to a multiple of 2 MB (aligned_alloc requires this)
    arr_bytes = (table_size * entry_size + HUGE_PAGE - 1) & ~(HUGE_PAGE - 1);

    arr = static_cast<TranspositionBucket *>(std::aligned_alloc(HUGE_PAGE, arr_bytes));
    if (!arr)
        throw std::bad_alloc();

    if (madvise(arr, arr_bytes, MADV_HUGEPAGE) != 0)
        perror("madvise");

    table_size_MB = table_size * entry_size / (1024 * 1024);
    clear();  // memset touches every page
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
    std::array<Move, max_legal_moves> legal;  // or reuse a scratch buffer
    int pre_len = stack.len();

    for (int i = 0; i <= depth; i++) {
        uint64_t hash = ZobroistHasher::get().hash_board(board);
        std::optional<TranspositionEntry> entry = get(hash);
        if (!entry || !entry->is_valid_move())
            break;

        const bool white = board.get_turn_color() == pieces::white;
        int n = white ? board.get_moves<full_search, true>(legal) : board.get_moves<full_search, false>(legal);

        const Move m = entry->bestmove;
        bool is_legal = false;
        for (int j = 0; j < n; j++) {
            if (m == legal[j]) {
                is_legal = true;
                break;
            }
        }
        if (!is_legal)
            break;  // stale entry or hash collision: stop the PV here

        RestoreMoveInfo info = white ? board.do_move<true>(m) : board.do_move<false>(m);
        pv_line.push_back(m);
        restore_stack.push(info);
        stack.push({ZobroistHasher::get().hash_board(board), board.get_num_pieces()});

        if (EvalState::forced_draw_ply(board) || stack.atleast_num(stack.top(), 2))
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
    hits = 0;        // how many times did we access the table and find the board inside?
    misses = 0;      // how many times did we access the table and not find the board?
    collisions = 0;  // how many times did we have a hash collision?
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
        return;  // deeper depth already exist for this state.
    }

    if (!dp_valid || (static_cast<uint16_t>(age) + depth) >= (static_cast<uint16_t>(dp.depth) + dp.age)) {  // different board states => age matters.
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