#pragma once
#include "core/board.h"
#include "core/piece.h"
#include "utils/config.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <random>

class ZobroistHasher {
  private:
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
    ZobroistHasher();

    std::array<std::array<uint64_t, 64>, 12> piece_numbers; // one number for each piece and square
    std::array<uint64_t, 16> castle_numbers;                // one for each castle combination
    std::array<uint64_t, 8> ep_numbers;                     // file of ep
    uint64_t black_number;                                  // indicate if black is playing or not.
    /**
     * @brief Adds to hash for a piece
     *
     * @tparam Piece_t type of piece
     * @tparam iswhite color of piece
     * @param[inout] hash Hash as input. new hash as output
     * @param[in] board to hash.
     */
    template <Piece_t p, bool is_white> void hash_piece(uint64_t &hash, const Board &board) const;
    template <Piece_t p> void hash_both_piece(uint64_t &hash, const Board &board) const;
    template <Piece_t p, bool is_white> constexpr static uint8_t piece_key() {
        uint8_t col_offset = is_white ? 0 : 6;
        return col_offset + (p - 1);
    }
    void hash_ep(uint64_t &hash, const Board &board) const;
    void hash_castle(uint64_t &hash, const Board &board) const;
    void hash_turn(uint64_t &hash, const Board &board) const;

    uint64_t rand_uint64_t();
    /**
     * @brief Generate hash from a board
     *
     * @param[in] board Board to hash
     * @return hash of board.
     */
    uint64_t hash_board(const Board &board) const;
    uint64_t delta(const HashDelta &delta) const;
    /**
     * @brief Hash can be incremented by XORing the parts that changed. This is color and pieces
     * moved and castle information.
     *
     * @tparam forward true if move is forwards in time, false if backwards (undo).
     * @param[in] hash hash updated with the move.
     */
    template <bool forward> void update_hash(uint64_t &hash, const Move move) {}
};