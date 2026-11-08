// Copyright 2025 Filip Agert
#include <bitboard.h>
#include <cstdlib>
#include <piece.h>
#include <random>
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
template <Piece_t p, bool is_white> void ZobroistHasher::hash_piece(uint64_t &hash, const Board &board) {
    BB piece_bb = board.get_piece_bb<p, is_white>();
    constexpr uint8_t key = piece_key<p, is_white>();
    BitLoop(piece_bb) {
        uint8_t LSB = BitBoard::lsb(piece_bb);
        uint64_t rand = piece_numbers[key][LSB];
        hash ^= rand;
    }
}
template <Piece_t p> void ZobroistHasher::hash_both_piece(uint64_t &hash, const Board &board) {
    hash_piece<p, true>(hash, board);
    hash_piece<p, false>(hash, board);
}
void ZobroistHasher::hash_ep(uint64_t &hash, const Board &board) {
    if (board.get_en_passant()) {
        uint8_t ep_file = NotationInterface::col(board.get_en_passant_square());
        hash ^= ep_numbers[ep_file];
    }
}
uint64_t ZobroistHasher::hash_board(const Board &board) {
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
    uint8_t col_offset = ((piece & pieces::color_mask) == pieces::black) ? 6 : 0;
    return col_offset + (piece & pieces::piece_mask) - 1;
}

uint64_t ZobroistHasher::delta(const HashDelta&d) const{
    uint64_t h = black_number;  // turn flips every move
    h ^= piece_numbers[key_of(d.moved_piece)][d.from];
    h ^= piece_numbers[key_of(d.placed_piece)][d.to];
    if (d.captured_piece)
        h ^= piece_numbers[key_of(d.captured_piece)][d.capture_sq];
    if (d.rook_piece) {
        uint8_t rk = key_of(d.rook_piece);
        h ^= piece_numbers[rk][d.rook_from] ^ piece_numbers[rk][d.rook_to];
    }
    h ^= castle_numbers[d.old_castle] ^ castle_numbers[d.new_castle];
    if (d.old_ep) h ^= ep_numbers[NotationInterface::col(d.old_ep)];
    if (d.new_ep) h ^= ep_numbers[NotationInterface::col(d.new_ep)];
    return h;
}
std::optional<TranspositionEntry> TranspositionTable::get(uint64_t hash) {
    size_t key = get_key(hash);
    TranspositionEntry curr = arr[key];
    std::optional<TranspositionEntry> maybe;
    if (curr.hash == (hash >> TranspositionEntry::shift_hash) && curr.nodetype != invalid) {
        hits++;
        maybe = std::make_optional(curr);
    } else {
        maybe = {};
        misses++;
        if (curr.hash != 0) {
            collisions++;
        }
    }
    return maybe;
}
int TranspositionTable::load_factor() const { return writes > 0 ? (overwrites * 1000) / writes : 0; }
