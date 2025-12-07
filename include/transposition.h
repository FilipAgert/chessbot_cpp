// Copyright 2025 Filip Agert
#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include <board.h>
#include <cstdint>
#include <random>
struct board_hash {
    uint64_t hash;
};

struct transposition_entry {
    board_hash hash;
    int eval;
};

class ZobroistHasher {
 private:
    std::array<std::array<uint64_t, 64>, 12> piece_numbers;  // one number for each piece and square
    std::array<uint64_t, 16> castle_numbers;                 // one for each castle combination
    std::array<uint64_t, 8> ep_numbers;                      // file of ep
    uint64_t black_number;                                   // indicate if black is playing or not.
    static std::uniform_int_distribution<uint64_t> generator;
    static std::mt19937_64 engine;
    static uint64_t random_uint64_t();
    static void initialize_engine();
    static const bool s_initialized;

 public:
    /**
     * @brief Generate hash from a board
     *
     * @param[in] board Board to hash
     * @return hash of board.
     */
    board_hash hash_board(Board &board);
    /**
     * @brief Hash can be incremented by XORing the parts that changed. This is color and pieces
     * moved and castle information.
     *
     * @param[[TODO:direction]] hash [TODO:description]
     */
    void update_hash(board_hash &hash, Move move);
};
#endif
