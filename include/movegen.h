// Copyright 2025 Filip Agert
#ifndef MOVEGEN_H
#define MOVEGEN_H
#include <bitboard.h>
namespace movegen {
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
 * @param[in] enemy_bb Bitboard of enemy pieces. These will be captured by the rook ray but block
 * further progress
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
