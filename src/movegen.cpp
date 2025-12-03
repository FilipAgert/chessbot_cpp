// Copyright 2025 Filip Agert
#include <iostream>
#include <movegen.h>
using namespace dirs;
using namespace masks;
namespace movegen {

uint64_t pawn_forward_moves(const uint64_t pawn_bb, const uint64_t all_bb, const uint8_t pawn_col) {
    int dir =
        (pawn_col == pieces::white) * N + (pawn_col == pieces::black) * S;  // branchless assignment
    uint8_t rowi =
        1 * (pawn_col == pieces::white) +
        6 * (pawn_col ==
             pieces::black);  // Branchless execution. Evaluates to 1 or 7 depending on color.
    uint64_t blocker = BitBoard::shift_bb(all_bb, -dir);           // Blocking pieces are one up
    uint64_t moves = BitBoard::shift_bb(pawn_bb & ~blocker, dir);  // All one moves up
    blocker = blocker | BitBoard::shift_bb(blocker, -dir);         // Pieces now block two moves...
    moves |= BitBoard::shift_bb(pawn_bb & ~blocker & row(rowi), dir, 2);
    return moves;
}
uint64_t pawn_attack_moves(const uint64_t pawn_bb, const uint64_t enemy_bb, const uint64_t ep_bb,
                           const uint8_t pawn_col) {
    return pawn_atk_bb(pawn_bb, pawn_col) &
           (enemy_bb | ep_bb);  // Require enemy or en passant there.
}
uint64_t pawn_moves(const uint64_t pawn_bb, const uint64_t friendly_bb, const uint64_t enemy_bb,
                    const uint64_t ep_bb, const uint8_t pawn_col) {
    return pawn_forward_moves(pawn_bb, friendly_bb | enemy_bb, pawn_col) |
           pawn_attack_moves(pawn_bb, enemy_bb, ep_bb, pawn_col);
}

uint64_t rook_atk(const uint8_t sq, const uint64_t occ) {
    uint64_t rook_bb = BitBoard::one_high(sq);
    uint64_t hit = ray(rook_bb, N, occ);
    hit |= ray(rook_bb, S, occ);
    hit |= rank_atk_bb(sq, occ);
    return hit;
}

uint64_t rook_moves_sq(const uint8_t rook_loc, const uint64_t friendly_bb,
                       const uint64_t enemy_bb) {
    uint64_t occ = friendly_bb | enemy_bb;
    uint64_t hit = rook_atk(rook_loc, occ);
    return hit & ~friendly_bb;
}
uint64_t rook_atk_bb(uint64_t rook_bb, const uint64_t occ) {
    uint64_t hit = ray(rook_bb, N, occ);
    hit |= ray(rook_bb, S, occ);
    uint8_t rook_sq = 0;
    uint64_t rook_bb_cp = rook_bb;
    while (rook_bb_cp > 0) {
        uint8_t temp = BitBoard::lsb(rook_bb_cp);  // Extract LSB loc.
        rook_sq += temp;
        rook_bb_cp = (rook_bb_cp >> temp) & ~1;  // Clear LSB
        hit |= rank_atk_bb(rook_sq, occ);
    }
    return hit;
}

uint64_t rook_moves_bb(const uint64_t rook_bb, const uint64_t friendly_bb,
                       const uint64_t enemy_bb) {
    return ~friendly_bb & rook_atk_bb(rook_bb, friendly_bb | enemy_bb);
}
uint64_t bishop_moves(const uint64_t bishop_bb, const uint64_t friendly_bb,
                      const uint64_t enemy_bb) {
    uint64_t all = friendly_bb | enemy_bb;
    uint64_t hit = ray(bishop_bb, NE, all);
    hit |= ray(bishop_bb, SE, all);
    hit |= ray(bishop_bb, SW, all);
    hit |= ray(bishop_bb, NW, all);
    return hit & ~friendly_bb;
}
uint64_t queen_moves(const uint64_t queen_bb, const uint64_t friendly_bb, const uint64_t enemy_bb) {
    return (~friendly_bb & rook_atk_bb(queen_bb, friendly_bb | enemy_bb)) |
           bishop_moves(queen_bb, friendly_bb, enemy_bb);
}
uint64_t king_moves(const uint8_t king_loc, const uint64_t friendly_bb, const uint64_t all_bb,
                    const uint64_t enemy_atk_bb, const uint8_t castle, const uint8_t turn_color) {
    uint64_t king_bb = BitBoard::one_high(king_loc);
    return king_castle_moves(king_bb, all_bb, enemy_atk_bb, castle, turn_color) |
           (king_attack_table[king_loc] & ~(friendly_bb | enemy_atk_bb));
}

uint64_t king_castle_moves(const uint64_t king_bb, const uint64_t all_bb,
                           const uint64_t enemy_atk_bb, const uint8_t castle,
                           const uint8_t turn_color) {
    // Castle flag is 0bxxxx
    //                  qkQK. First, check if our side can castle.
    const uint8_t castle_flag =
        0b0011 * (turn_color == pieces::white) + 0b1100 * (turn_color == pieces::black);
    if ((castle_flag & castle) == 0)
        return 0;
    const uint8_t kingside = 0b0101;
    const uint8_t queenside = 0b1010;
    const uint64_t kingside_atk_board =  // No pieces may attack these positions
        0b0111000000000000000000000000000000000000000000000000000000000000 *
            (turn_color == pieces::black) +
        0b0000000000000000000000000000000000000000000000000000000001110000 *
            (turn_color == pieces::white);
    const uint64_t kingside_move_board =  // No pieces may block these positions
        0b0110000000000000000000000000000000000000000000000000000000000000 *
            (turn_color == pieces::black) +
        0b0000000000000000000000000000000000000000000000000000000001100000 *
            (turn_color == pieces::white);
    const uint64_t queenside_move_board =  // No pieces may block these positions
        0b0000111000000000000000000000000000000000000000000000000000000000 *
            (turn_color == pieces::black) +
        0b0000000000000000000000000000000000000000000000000000000000001110 *
            (turn_color == pieces::white);
    const uint64_t queenside_atk_board =  // No pieces may atk these positions
        0b0001110000000000000000000000000000000000000000000000000000000000 *
            (turn_color == pieces::black) +
        0b0000000000000000000000000000000000000000000000000000000000011100 *
            (turn_color == pieces::white);
    const uint64_t kingside_to =  // Where king moves kingside
        0b0100000000000000000000000000000000000000000000000000000000000000 *
            (turn_color == pieces::black) +
        0b0000000000000000000000000000000000000000000000000000000001000000 *
            (turn_color == pieces::white);
    const uint64_t queenside_to =  // Where king moves queenside
        0b0000010000000000000000000000000000000000000000000000000000000000 *
            (turn_color == pieces::black) +
        0b0000000000000000000000000000000000000000000000000000000000000100 *
            (turn_color == pieces::white);
    // Now we know we can castle. Get castle moves as branchless execution

    uint64_t castle_moves =
        ((castle_flag & kingside & castle) > 0 && ((kingside_atk_board & enemy_atk_bb) == 0) &&
         ((kingside_move_board & all_bb) == 0)) *
            kingside_to +
        ((castle_flag & queenside & castle) > 0 && ((queenside_atk_board & enemy_atk_bb) == 0) &&
         ((queenside_move_board & all_bb) == 0)) *
            queenside_to;
    return castle_moves;
}
}  // namespace movegen
