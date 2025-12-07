// Copyright 2025 Filip Agert
#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include <bitboard.h>
#include <board.h>
#include <cstdint>
#include <piece.h>
#include <random>

struct transposition_entry {
    uint64_t hash;
    int eval;
};

class ZobroistHasher {
 private:
    static std::array<std::array<uint64_t, 64>, 12>
        piece_numbers;                               // one number for each piece and square
    static std::array<uint64_t, 16> castle_numbers;  // one for each castle combination
    static std::array<uint64_t, 8> ep_numbers;       // file of ep
    static uint64_t black_number;                    // indicate if black is playing or not.
    static std::uniform_int_distribution<uint64_t> generator;
    static std::mt19937_64 engine;
    /**
     * @brief Generates a random uint64_t number. Seeded at startup so will be different each
     * session.
     *
     * @return random uint64_t
     */
    static uint64_t rand_uint64_t();
    static void initialize_engine();
    static void randomize_local_vars();
    static const bool s_initialized;

    /**
     * @brief Adds to hash for a piece
     *
     * @tparam Piece_t type of piece
     * @tparam iswhite color of piece
     * @param[inout] hash Hash as input. new hash as output
     * @param[in] board to hash.
     */
    template <Piece_t p, bool is_white> static void hash_piece(uint64_t &hash, const Board &board);
    template <Piece_t p> static void hash_both_piece(uint64_t &hash, const Board &board);
    template <Piece_t p, bool is_white> constexpr static uint8_t piece_key() {
        uint8_t col_offset = is_white ? 0 : 6;
        return col_offset + (p - 1);
    }
    static void hash_ep(uint64_t &hash, const Board &board);
    static void hash_castle(uint64_t &hash, const Board &board) { hash ^= board.get_castling(); }
    static void hash_turn(uint64_t &hash, const Board &board) {
        if (board.get_turn_color() == pieces::black) {
            hash ^= ZobroistHasher::black_number;
        }
    }

 public:
    /**
     * @brief Generate hash from a board
     *
     * @param[in] board Board to hash
     * @return hash of board.
     */
    static uint64_t hash_board(Board &board);
    /**
     * @brief Hash can be incremented by XORing the parts that changed. This is color and pieces
     * moved and castle information.
     *
     * @param[[TODO:direction]] hash [TODO:description]
     */
    static void update_hash(uint64_t &hash, Move move);
};
#endif
