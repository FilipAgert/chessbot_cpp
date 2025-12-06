// Copyright 2025 Filip Agert
#include <iostream>
#include <movegen.h>
using namespace dirs;
using namespace masks;
namespace magic {
const std::array<uint64_t, rook_magic_table_sz> rook_magic_bitboards =
    [] {  // precompute the magic bitboards;
        std::array<uint64_t, rook_magic_table_sz> rook_magic_bbs;
        for (int sq = 0; sq < 64; sq++) {
            std::array<uint64_t, max_size> occ, atk;
            uint64_t occmask = occupancy_bits_rook(sq);
            occ = gen_occ_variation(occmask, rook_num_occ_bits[sq]);
            atk = compute_atk_bbs(occ, sq, true);
            int nvars = 1 << rook_num_occ_bits[sq];

            for (int i = 0; i < nvars; i++) {
                int idx = get_rook_magic_idx(sq, occ[i]);
                rook_magic_bbs[idx] = atk[i];
            }
        }
        return rook_magic_bbs;
    }();
const std::array<uint64_t, bishop_magic_table_sz> bishop_magic_bitboards =
    [] {  // precompute the magic bitboards;
        std::array<uint64_t, bishop_magic_table_sz> bishop_magic_bbs;
        for (int sq = 0; sq < 64; sq++) {
            std::array<uint64_t, max_size> occ, atk;
            uint64_t occmask = occupancy_bits_bishop(sq);
            occ = gen_occ_variation(occmask, bishop_num_occ_bits[sq]);
            atk = compute_atk_bbs(occ, sq, false);
            int nvars = 1 << bishop_num_occ_bits[sq];

            for (int i = 0; i < nvars; i++) {
                int idx = get_bishop_magic_idx(sq, occ[i]);
                bishop_magic_bbs[idx] = atk[i];
            }
        }
        return bishop_magic_bbs;
    }();

}  // namespace magic
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

uint64_t king_moves(const uint8_t king_loc, const uint64_t friendly_bb, const uint64_t all_bb,
                    const uint64_t enemy_atk_bb, const uint8_t castle, const uint8_t turn_color) {
    uint64_t king_bb = BitBoard::one_high(king_loc);
    return king_castle_moves(king_bb, all_bb, enemy_atk_bb, castle, turn_color) |
           (king_atk(king_loc) & ~(friendly_bb | enemy_atk_bb));
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
