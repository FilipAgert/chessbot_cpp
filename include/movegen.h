// Copyright 2025 Filip Agert
#ifndef MOVEGEN_H
#define MOVEGEN_H
#include <bitboard.h>
#include <iostream>
#include <notation_interface.h>
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
template <class T, uint8_t N, T (*F)(uint8_t)>
constexpr std::array<T, N> generate_simple_move_table_uint8_t() {
    std::array<T, N> table{};
    for (uint8_t i = 0; i < N; ++i) {
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
constexpr uint64_t king_atk_sq(const uint8_t sq) {
    uint64_t king_bb = BitBoard::one_high(sq);
    return king_atk_bb(king_bb);
}
constexpr uint64_t knight_atk_sq(const uint8_t sq) {
    uint64_t knight_bb = BitBoard::one_high(sq);
    return knight_atk_bb(knight_bb);
}

constexpr uint64_t bishop_atk(const uint64_t bishop_bb, const uint64_t occ) {
    uint64_t hit = ray(bishop_bb, NE, occ);
    hit |= ray(bishop_bb, SE, occ);
    hit |= ray(bishop_bb, SW, occ);
    hit |= ray(bishop_bb, NW, occ);
    return hit;
}

constexpr std::array<uint64_t, 64> king_attack_table =  // 512 bytes
    generate_simple_move_table_uint8_t<uint64_t, 64, king_atk_sq>();
constexpr std::array<uint64_t, 64> knight_attack_table =  // 512 bytes
    generate_simple_move_table_uint8_t<uint64_t, 64, knight_atk_sq>();

// MAGIC BITBOARDFUCKERY
/**
 * @brief Gets the relevant occupancy when constructing a magic bitboard.
 *
 * @param[in] sq square of rook
 * @return relevant occupancy bits are set to high.
 */
constexpr uint64_t occupancy_bits_rook(uint8_t sq) {
    uint8_t row = NotationInterface::row(sq);
    uint8_t col = NotationInterface::col(sq);
    uint64_t occbits =
        (masks::col(col) | masks::row(row)) & (~(masks::around | BitBoard::one_high(sq)));
    return occbits;
}

constexpr std::array<uint64_t, 64> rook_occupancy_table =
    generate_simple_move_table_uint8_t<uint64_t, 64, occupancy_bits_rook>();

constexpr uint64_t occupancy_bits_bishop(uint8_t sq) {
    uint64_t bb = BitBoard::one_high(sq);
    return bishop_atk(bb, 0) & ~(masks::around | BitBoard::one_high(sq));
}
constexpr std::array<uint64_t, 64> bishop_occupancy_table =
    generate_simple_move_table_uint8_t<uint64_t, 64, occupancy_bits_bishop>();

/**
 * @brief Generates bitboard of possible moves for a rook on the first rank (1).
 *
 * @param[in] bb Bitboard representing occupancy in the 6 middle columns.
 * @param[in] col Column of rook.
 * @return Bitboard representing the attacking moves for the rook.
 */
constexpr uint8_t first_rank_atk_bb(const int idx) {
    // int idx = 8 * occ + col;
    uint64_t col = idx % 8;
    uint64_t occ = (idx - col) / 8;

    uint64_t origin = BitBoard::one_high(col);
    uint64_t blockers = occ << 1;  // rightshift by one.

    uint8_t res = ray(origin, dirs::E, blockers) | ray(origin, dirs::W, blockers);
    return res;
}

// Array storing the attacks of the first rank for rooks. Indexed by a*b where b is the rook
// position (column) and [a] is the 6 bit bitboard for occupancy in the middle 6 columns. In
// practice, take the 64 bit piece bitboard, shift 1 bit to the right and take first 6 bits as
// index a.
constexpr std::array<uint8_t, 512> first_rank_attack_table =  // 512 bytes
    generate_simple_move_table<uint8_t, 512, first_rank_atk_bb>();

/**
 * @brief Gets the attack bitboard for the current rank (row) given an occupancy bitboard
 *
 * @param[in] occ bitboard of occupancy of all pieces
 * @param[in] sq square of rook
 * @return attack bitboard
 */
constexpr uint64_t rank_atk_bb(uint8_t sq, uint64_t occ) {
    int rowx8 = NotationInterface::row(sq) * 8;
    int col = NotationInterface::col(sq);
    uint64_t occrank = ((occ >> rowx8) & 0b01111110) >> 1;
    // The mask 0b1111110 masks out middle 6 columns
    // Shifts the occupancy bitboard down to the first rank. Mask out the middle 6 bits from this to
    // obtain index. Finally bitshift by one to the right a mask for the middle six squares in the
    // first rank. Then bitshift it by one to the right.
    return (uint64_t)first_rank_attack_table[occrank * 8 + col] << rowx8;
}
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
 * @param[in] rook_loc Location of rook.
 * @param[in] friendly_bb Bitboard of friendly pieces. These will block the rook ray.
 * @param[in] enemy_bb Bitboard of enemy pieces. These will be captured by the rook ray but
 * block further progress
 * @return Bitboard with all available move squares set to 1.
 */
uint64_t rook_moves_sq(const uint8_t rook_loc, const uint64_t friendly_bb, const uint64_t enemy_bb);
uint64_t rook_moves_bb(const uint64_t rook_bb, const uint64_t friendly_bb, const uint64_t enemy_bb);
/**
 * @brief Generate all squares that all rooks atk
 *
 * @param[in] rook_bb bitboard of rooks
 * @param[in] occ occupancy bitboard
 * @return bitboard of all attacked squares by rooks
 */
uint64_t rook_atk_bb(uint64_t rook_bb, const uint64_t occ);
/**
 * @brief Gets squares threatened by a rook at a square
 *
 * @param[in] sq Square of rook
 * @param[in] occ Occupancy bb
 * @return [TODO:description]
 */
uint64_t rook_atk(const uint8_t sq, const uint64_t occ);
uint64_t bishop_moves(const uint64_t bishop_bb, const uint64_t friendly_bb,
                      const uint64_t enemy_bb);
uint64_t queen_moves(const uint64_t queen_bb, const uint64_t friendly_bb, const uint64_t enemy_bb);
uint64_t pawn_moves(const uint64_t pawn_bb, const uint64_t friendly_bb, const uint64_t enemy_bb,
                    const uint64_t ep_bb, const uint8_t pawn_color);
uint64_t pawn_attack_moves(const uint64_t pawn_bb, const uint64_t enemy_bb, const uint64_t ep_bb,
                           const uint8_t pawn_col);

}  // namespace movegen

#endif  // MOVEGEN_H
