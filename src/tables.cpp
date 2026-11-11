// Copyright 2025 Filip Agert
#include "config.h"
#include <bitboard.h>
#include <cassert>
#include <cstdlib>
#include <piece.h>
#include <random>
#include <stdexcept>
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
    uint64_t h = black_number;  // turn flips every move
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
    size_t num_entries = desired_size_MB * 1000000 / entry_size;
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
    int table_size = 1ULL << numbits;
    if (!(numbits > TranspositionEntry::shift_hash))
        throw std::runtime_error("must have more bits in table than in the hash shift");
    mask = (1ULL << numbits) - 1;  // lowest nbits set high.

    table_size_MB = table_size * entry_size / 1000000;
    arr.resize(table_size);
}
std::optional<TranspositionEntry> TranspositionTable::get(uint64_t hash) {
    size_t key = get_key(hash);
    TranspositionBucket &curr = arr[key];
    std::optional<TranspositionEntry> maybe;
    uint64_t shifted = hash >> TranspositionEntry::shift_hash;
    if (curr.depth_pref.hash == shifted && curr.depth_pref.nodetype != invalid) {
        hits++;
        maybe = std::make_optional(curr.depth_pref);
    } else if (curr.always_replace.hash == shifted && curr.always_replace.nodetype != invalid) {
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
std::vector<Move> TranspositionTable::get_pv(Board &board, int depth) {
    std::vector<Move> pv_line;
    std::stack<RestoreMoveInfo> restore_stack;
    for (int i = 0; i <= depth; i++) {
        uint64_t hash = ZobroistHasher::get().hash_board(board);
        std::optional<TranspositionEntry> entry = get(hash);
        if (entry) {
            if (entry.value().is_valid_move()) {
                auto it = std::find(pv_line.begin(), pv_line.end(), entry.value().bestmove);  // check if move already found.
                if (it != pv_line.end()) {
                    break;  // found move inside.
                } else {
                    pv_line.push_back(entry.value().bestmove);
                    RestoreMoveInfo info;
                    if (board.get_turn_color() == pieces::white)
                        info = board.do_move<true>(entry.value().bestmove);
                    else
                        info = board.do_move<false>(entry.value().bestmove);
                    restore_stack.push(info);
                    if (EvalState::forced_draw_ply(board))
                        break;
                }
            }
        }
    }
    for (int i = pv_line.size() - 1; i >= 0; i--) {
        RestoreMoveInfo info = restore_stack.top();
        restore_stack.pop();
        board.undo_move(info, pv_line[i]);
    }
    return pv_line;
}
int TranspositionTable::load_factor() const { return writes > 0 ? (overwrites * 1000) / writes : 0; }

void TranspositionTable::clear() {
    std::fill(arr.begin(), arr.end(), TranspositionBucket{TranspositionEntry{0, invalid, 0, 0, Move()}, TranspositionEntry{0, invalid, 0, 0, Move()}});
    writes = 0;
    overwrites = 0;
    hits = 0;        // how many times did we access the table and find the board inside?
    misses = 0;      // how many times did we access the table and not find the board?
    collisions = 0;  // how many times did we have a hash collision?
}

void TranspositionTable::store(uint64_t hash, Move bestmove, int eval, uint8_t nodetype, uint8_t depth) {
    uint64_t h = (hash >> TranspositionEntry::shift_hash);
    size_t key = get_key(hash);
    writes++;
    TranspositionBucket &bucket = arr[key];
    TranspositionEntry &dp = bucket.depth_pref;
    TranspositionEntry &ar = bucket.always_replace;
    bool dp_valid = dp.nodetype != nodetype::invalid;
    bool ar_valid = ar.nodetype != nodetype::invalid;
    // 1. Fill up buckets (Do not if same hash on both)
    // cases:
    // depth empty,  2 empty
    // depth full,   2 empty
    // depth full,   2 full

    TranspositionEntry ne{h, nodetype, depth, eval, bestmove};
    // benchmark never demoting.
    if (dp_valid && dp.hash == h) {
        if (depth >= dp.depth) {
            if (!ne.is_valid_move())
                ne.bestmove = dp.bestmove;
            dp = ne;
        }
        return;  // deeper depth already exist for this state.
    }

    if (!dp_valid || depth >= dp.depth) {
        if (dp_valid) {
            if (ar_valid && ar.hash != dp.hash)
                overwrites++;
            ar = dp;
            // move into always replace slot.
        } else if (ar_valid && ar.hash == h) {
            ar.nodetype = invalid;
            // invalidate.
        }

        dp = ne;
    } else {
        if (ar_valid && ar.hash != h)
            overwrites++;
        ar = ne;
    }
}