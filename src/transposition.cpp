// Copyright 2025 Filip Agert
#include <bitboard.h>
#include <cstdlib>
#include <piece.h>
#include <random>
#include <transposition.h>

std::array<std::array<uint64_t, 64>, 12>
    ZobroistHasher::piece_numbers{};                        // one number for each piece and square
std::array<uint64_t, 16> ZobroistHasher::castle_numbers{};  // one for each castle combination
std::array<uint64_t, 8> ZobroistHasher::ep_numbers{};       // file of ep
uint64_t ZobroistHasher::black_number;                      // indicate if black is playing or not.
std::uniform_int_distribution<uint64_t> ZobroistHasher::generator;
std::mt19937_64 ZobroistHasher::engine;

const bool ZobroistHasher::s_initialized = []() {
    ZobroistHasher::initialize_engine();
    ZobroistHasher::randomize_local_vars();
    return true;
};

void ZobroistHasher::initialize_engine() {
    std::random_device sd;
    ZobroistHasher::engine.seed(sd());
}
uint64_t ZobroistHasher::rand_uint64_t() { return generator(engine); }
void ZobroistHasher::randomize_local_vars() {
    for (int p = 0; p < 12; p++) {
        for (int sq = 0; sq < 64; sq++) {
            piece_numbers[p][sq] = ZobroistHasher::rand_uint64_t();
        }
    }
    for (int i = 0; i < 16; i++) {
        castle_numbers[i] = ZobroistHasher::rand_uint64_t();
    }
    for (int i = 0; i < 8; i++) {
        ep_numbers[i] = ZobroistHasher::rand_uint64_t();
    }
    black_number = ZobroistHasher::rand_uint64_t();
}
template <Piece_t p, bool is_white>
void ZobroistHasher::hash_piece(uint64_t &hash, const Board &board) {
    BB piece_bb = board.get_piece_bb<p, is_white>();
    constexpr uint8_t key = ZobroistHasher::piece_key<p, is_white>();
    BitLoop(piece_bb) {
        uint8_t LSB = BitBoard::lsb(piece_bb);
        uint64_t rand = ZobroistHasher::piece_numbers[key][LSB];
        hash ^= rand;
    }
}
template <Piece_t p> void ZobroistHasher::hash_both_piece(uint64_t &hash, const Board &board) {
    ZobroistHasher::hash_piece<p, true>(hash, board);
    ZobroistHasher::hash_piece<p, false>(hash, board);
}
void ZobroistHasher::hash_ep(uint64_t &hash, const Board &board) {
    if (board.get_en_passant()) {
        uint8_t ep_file = NotationInterface::col(board.get_en_passant_square());
        hash ^= ZobroistHasher::ep_numbers[ep_file];
    }
}
uint64_t ZobroistHasher::hash_board(const Board &board) {
    uint64_t hash = 0;
    ZobroistHasher::hash_both_piece<pieces::king>(hash, board);
    ZobroistHasher::hash_both_piece<pieces::queen>(hash, board);
    ZobroistHasher::hash_both_piece<pieces::rook>(hash, board);
    ZobroistHasher::hash_both_piece<pieces::bishop>(hash, board);
    ZobroistHasher::hash_both_piece<pieces::knight>(hash, board);
    ZobroistHasher::hash_both_piece<pieces::pawn>(hash, board);
    ZobroistHasher::hash_ep(hash, board);
    ZobroistHasher::hash_castle(hash, board);
    ZobroistHasher::hash_turn(hash, board);
    return hash;
}
