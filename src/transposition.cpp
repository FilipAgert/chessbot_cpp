// Copyright 2025 Filip Agert
#include <cstdlib>
#include <random>
#include <transposition.h>

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
