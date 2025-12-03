// Copyright 2025 Filip Agert
#ifndef MOVEGEN_H
#define MOVEGEN_H
#include <bitboard.h>
using namespace dirs;
using namespace masks;
namespace movegen {
/**
 * @brief Shoots ray from origin, up to edge of board and returns the corresponding bitboard.
 *
 * @param origin
 * @param dir
 * @return uint64_t
 */
constexpr uint64_t ray(const uint64_t origin, const int dir, const uint64_t blocker_bb,
                       const int steps) {
    // How to ensure no wrap-around?
    uint64_t hit = origin;
    uint64_t mask =
        edge_mask(dir) | blocker_bb;  // The edge mask ensures we do not wrap-around.The blocker
                                      // mask ensures that we do not keep going through somebody.
    hit |= BitBoard::shift_bb(
        ~edge_mask(dir) & hit,
        dir);  // Take first step without the blocker bb, since the blocker bb includes self.
    for (int i = 2; i <= steps; i++) {
        hit |= BitBoard::shift_bb(
            (~mask) & hit,
            dir);  // Shift the mask in dir direction, but only on non-masked places.
    }
    return hit & ~origin;  // Exclude origin, since the piece does not attack itself.
}
constexpr uint64_t ray(const uint64_t origin, const int dir, const uint64_t blocker_bb) {
    return ray(origin, dir, blocker_bb, 7);
}
constexpr uint64_t ray(const uint64_t origin, const int dir) { return ray(origin, dir, 0, 7); }
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
 * @brief Bitboard for knight attack moves from bitboard.
 *
 * @param[[TODO:direction]] sq [TODO:description]
 * @return [TODO:description]
 */
constexpr uint64_t knight_atk_bb(const uint64_t knight_bb) {
    uint64_t out = (knight_bb & (~(top | row(6))) & (~right)) << (N + N + E);  // NNE
    out |= (knight_bb & (~(top)) & (~(right | col(6)))) << (N + E + E);        // NEE
    out |= (knight_bb & (~(bottom)) & (~(right | col(6)))) >> -(S + E + E);    // SEE
    out |= (knight_bb & (~(bottom | row(1))) & (~(right))) >> -(S + S + E);    // SSE

    out |= (knight_bb & (~(bottom | row(1))) & (~(left))) >> -(S + S + W);  // SSW
    out |= (knight_bb & (~(bottom)) & (~(left | col(1)))) >> -(S + W + W);  // SWW
    out |= (knight_bb & (~(top)) & (~(left | col(1)))) << (N + W + W);      // NWW
    out |= (knight_bb & (~(top | row(6))) & (~left)) << (N + N + W);        // NNW
    return out;
}

/**
 * @brief Generates the squares the king threatens from its bitboard representation.
 *
 * @param[in] king_bb Bitboard representing position of king.
 * @return BB with bits high if king threatens this square.
 */
constexpr uint64_t king_atk_bb(const uint64_t king_bb) {
    uint64_t hit = ray(king_bb, N, 0, 1);
    hit |= ray(king_bb, NE, 0, 1);
    hit |= ray(king_bb, E, 0, 1);
    hit |= ray(king_bb, SE, 0, 1);
    hit |= ray(king_bb, S, 0, 1);
    hit |= ray(king_bb, SW, 0, 1);
    hit |= ray(king_bb, W, 0, 1);
    hit |= ray(king_bb, NW, 0, 1);
    return hit;
}
constexpr uint64_t pawn_atk_bb(const uint64_t pawn_bb, const uint8_t pawn_col) {
    int dir =
        (pawn_col == pieces::white) * N + (pawn_col == pieces::black) * S;  // branchless assignment
    uint64_t moves = BitBoard::shift_bb(pawn_bb & ~col(7), dir + 1) |
                     BitBoard::shift_bb(pawn_bb & ~col(0), dir - 1);
    return moves;
}
constexpr uint64_t king_atk_sq(const int sq) {
    uint64_t king_bb = BitBoard::one_high(sq);
    return king_atk_bb(king_bb);
}
constexpr uint64_t knight_atk_sq(const int sq) {
    uint64_t knight_bb = BitBoard::one_high(sq);
    return knight_atk_bb(knight_bb);
}

constexpr std::array<uint64_t, 64> king_attack_table =
    generate_simple_move_table<uint64_t, 64, king_atk_sq>();
constexpr std::array<uint64_t, 64> knight_attack_table =
    generate_simple_move_table<uint64_t, 64, knight_atk_sq>();
/**
 * @brief Gets the bitboard for the available knight moves.
 *
 * @param knight_loc Location of knight.
 * @param friendly_bb location of friendly pieces.
 * @return uint64_t Bitboard of all the knight moves.
 */
constexpr uint64_t knight_moves(const uint8_t knight_loc, const uint64_t friendly_bb) {
    return knight_attack_table[knight_loc] & ~friendly_bb;
}
uint64_t king_castle_moves(const uint64_t king_bb, const uint64_t all_bb,
                           const uint64_t enemy_atk_bb, const uint8_t castle,
                           const uint8_t turn_color);

/**
 * @brief Generates bitboard for all possible king moves. The 1s in output integer represents the
 * possible to squares the king can move to.
 *
 * @param[[TODO:direction]] king_loc [TODO:description]
 * @param[[TODO:direction]] friendly_bb [TODO:description]
 * @param[[TODO:direction]] all_bb [TODO:description]
 * @param[[TODO:direction]] enemy_atk_bb [TODO:description]
 * @param[[TODO:direction]] castle [TODO:description]
 * @param[[TODO:direction]] turn_color [TODO:description]
 * @return [TODO:description]
 */
uint64_t king_moves(const uint8_t king_loc, const uint64_t friendly_bb, const uint64_t all_bb,
                    const uint64_t enemy_atk_bb, const uint8_t castle, const uint8_t turn_color);
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
uint64_t pawn_moves(const uint64_t pawn_bb, const uint64_t friendly_bb, const uint64_t enemy_bb,
                    const uint64_t ep_bb, const uint8_t pawn_color);
uint64_t pawn_attack_moves(const uint64_t pawn_bb, const uint64_t enemy_bb, const uint64_t ep_bb,
                           const uint8_t pawn_col);

}  // namespace movegen

#endif  // MOVEGEN_H
