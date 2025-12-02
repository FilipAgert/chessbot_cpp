// Copyright 2025 Filip Agert
#ifndef MOVEGEN_H
#define MOVEGEN_H
#include <bitboard.h>
using namespace dirs;
using namespace masks;
namespace movegen {
/**
 * @brief Lambda to help generate attack tables in compile time.
 *
 * @tparam T [TODO:description]
 * @tparam N [TODO:description]
 * @tparam T [TODO:description]
 */
template <class T, size_t N, T (*F)(int)> constexpr std::array<T, N> generate_simple_move_table() {
    std::array<T, N> table{};
    for (size_t i = 0; i < N; ++i) {
        // The compile-time loop populates the array
        table[i] = F(i);
    }
    return table;
}
/**
 * @brief Bitboard for knight attack moves from sq.
 *
 * @param[[TODO:direction]] sq [TODO:description]
 * @return [TODO:description]
 */
constexpr uint64_t knight_atk_bb(int sq) {
    uint64_t knight_loc = BitBoard::one_high(sq);

    uint64_t out = (knight_loc & (~(top | row(6))) & (~right)) << (N + N + E);  // NNE
    out |= (knight_loc & (~(top)) & (~(right | col(6)))) << (N + E + E);        // NEE
    out |= (knight_loc & (~(bottom)) & (~(right | col(6)))) >> -(S + E + E);    // SEE
    out |= (knight_loc & (~(bottom | row(1))) & (~(right))) >> -(S + S + E);    // SSE

    out |= (knight_loc & (~(bottom | row(1))) & (~(left))) >> -(S + S + W);  // SSW
    out |= (knight_loc & (~(bottom)) & (~(left | col(1)))) >> -(S + W + W);  // SWW
    out |= (knight_loc & (~(top)) & (~(left | col(1)))) << (N + W + W);      // NWW
    out |= (knight_loc & (~(top | row(6))) & (~left)) << (N + N + W);        // NNW
    return out;
}

constexpr uint64_t king_atk_bb(int sq) { return 1; }  // FIX: must implement
constexpr std::array<uint64_t, 64> king_attack_table =
    generate_simple_move_table<uint64_t, 64, king_atk_bb>();
constexpr std::array<uint64_t, 64> knight_attack_table =
    generate_simple_move_table<uint64_t, 64, knight_atk_bb>();
/**
 * @brief Gets the bitboard for the available knight moves.
 *
 * @param knight_loc Location of knights.
 * @param friendly_bb location of friendly pieces.
 * @return uint64_t Bitboard of all the knight moves.
 */
uint64_t knight_moves(const uint64_t knight_bb, const uint64_t friendly_bb);
/**
 * @brief Generates all moves for rooks given a bitboard of its location
 * @param[in] rook_bb Bitboard of rooks
 * @param[in] friendly_bb Bitboard of friendly pieces. These will block the rook ray.
 * @param[in] enemy_bb Bitboard of enemy pieces. These will be captured by the rook ray but
 * block further progress
 * @return Bitboard with all available move squares set to 1.
 */
uint64_t rook_moves(const uint64_t rook_bb, const uint64_t friendly_bb, const uint64_t enemy_bb);
uint64_t bishop_moves(const uint64_t bishop_bb, const uint64_t friendly_bb,
                      const uint64_t enemy_bb);
uint64_t queen_moves(const uint64_t queen_bb, const uint64_t friendly_bb, const uint64_t enemy_bb);
uint64_t king_moves(const uint64_t king_bb, const uint64_t friendly_bb, const uint64_t all_bb,
                    const uint64_t enemy_atk_bb, const uint8_t castle, const uint8_t turn_color);
uint64_t king_move_moves(const uint64_t king_bb, const uint64_t friendly_bb,
                         const uint64_t enemy_atk_bb);
uint64_t king_castle_moves(const uint64_t king_bb, const uint64_t all_bb,
                           const uint64_t enemy_atk_bb, const uint8_t castle,
                           const uint8_t turn_color);
uint64_t pawn_moves(const uint64_t pawn_bb, const uint64_t friendly_bb, const uint64_t enemy_bb,
                    const uint64_t ep_bb, const uint8_t pawn_color);
uint64_t pawn_threaten_moves(const uint64_t pawn_bb,
                             const uint8_t pawn_col);  // Squares THREATENED. By pawns.
uint64_t pawn_attack_moves(const uint64_t pawn_bb, const uint64_t enemy_bb, const uint64_t ep_bb,
                           const uint8_t pawn_col);

/**
 * @brief Shoots ray from origin, up to edge of board and returns the corresponding bitboard.
 *
 * @param origin
 * @param dir
 * @return uint64_t
 */
uint64_t ray(const uint64_t origin, const int dir, const uint64_t blocker_bb, const int steps);
uint64_t ray(const uint64_t origin, const int dir, const uint64_t blocker_bb);
uint64_t ray(const uint64_t origin, const int dir);
}  // namespace movegen

#endif  // MOVEGEN_H
