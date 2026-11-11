// Copyright 2025 Filip Agert
#include <iostream>
#include <movegen.h>
using namespace dirs;
using namespace masks;
namespace magic {
std::array<uint64_t, max_size> compute_atk_bbs(std::array<uint64_t, max_size> occ_vars, int sq, bool rook, bool xray) {
    int m = rook ? rook_num_occ_bits[sq] : bishop_num_occ_bits[sq];
    int num = 1 << m;  // number of occ boards to generate.
    std::array<uint64_t, max_size> atk_bbs;
    for (int n = 0; n < num; n++) {
        if (xray) {
            atk_bbs[n] = rook ? rook_xray_atk_bb_helper(sq, occ_vars[n]) : bishop_xray_atk_bb_helper(sq, occ_vars[n]);
        } else {
            atk_bbs[n] = rook ? rook_atk_bb_helper(sq, occ_vars[n]) : bishop_atk_bb_helper(sq, occ_vars[n]);
        }
    }
    return atk_bbs;
}
std::array<uint64_t, max_size> gen_occ_variation(uint64_t occ_mask, int m) {
    // 1: Find binary rep of n
    // 2: For each 1 in n, starting from the right:
    //    Put it on each 1 in occ_mask, in the correct order.
    int num = 1 << m;  // number of occ boards to generate
    std::array<uint8_t, max_bits> mask_one_locations = mask_one_locs(occ_mask);
    std::array<uint64_t, max_size> occ_vars;

    for (int n = 0; n < num; n++) {
        int nreduced = n;
        uint64_t out_nbr = 0;
        while (nreduced > 0) {
            uint8_t hibit = BitBoard::lsb(nreduced);  // Extract LSB loc.
            nreduced = BitBoard::clear_lsb(nreduced);
            // hi_bit, m, is then the location of the current high bit m in the number n. This
            // should be put on the mth 1 in occ_mask. How to find the location of the mth one in
            // occ_mask? Do the same.
            uint8_t mask_one_loc = mask_one_locations[hibit];
            out_nbr |= BitBoard::one_high(mask_one_loc);
        }
        occ_vars[n] = out_nbr;
    }
    return occ_vars;
}

alignas(64) const std::array<uint64_t, rook_magic_table_sz> rook_magic_bitboards = [] {  // precompute the magic bitboards;
    std::array<uint64_t, rook_magic_table_sz> rook_magic_bbs;
    for (int sq = 0; sq < 64; sq++) {
        std::array<uint64_t, max_size> occ, atk;
        uint64_t occmask = occupancy_bits_rook(sq);
        occ = gen_occ_variation(occmask, rook_num_occ_bits[sq]);
        atk = compute_atk_bbs(occ, sq, true, false);
        int nvars = 1 << rook_num_occ_bits[sq];

        for (int i = 0; i < nvars; i++) {
            int idx = get_rook_magic_idx(sq, occ[i]);
            rook_magic_bbs[idx] = atk[i];
        }
    }
    return rook_magic_bbs;
}();
alignas(64) const std::array<uint64_t, bishop_magic_table_sz> bishop_magic_bitboards = [] {  // precompute the magic bitboards;
    std::array<uint64_t, bishop_magic_table_sz> bishop_magic_bbs;
    for (int sq = 0; sq < 64; sq++) {
        std::array<uint64_t, max_size> occ, atk;
        uint64_t occmask = occupancy_bits_bishop(sq);
        occ = gen_occ_variation(occmask, bishop_num_occ_bits[sq]);
        atk = compute_atk_bbs(occ, sq, false, false);
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

uint64_t king_moves(const uint8_t king_loc, const uint64_t friendly_bb, const uint64_t all_bb, const uint64_t enemy_atk_bb, const uint8_t castle,
                    const uint8_t turn_color) {
    uint64_t king_bb = BitBoard::one_high(king_loc);
    return (king_castle_moves(king_bb, all_bb, enemy_atk_bb, castle, turn_color) | (king_atk(king_loc))) & (~(friendly_bb | enemy_atk_bb));
}

uint64_t king_castle_moves(const uint64_t king_bb, const uint64_t all_bb, const uint64_t enemy_atk_bb, const uint8_t castle, const uint8_t turn_color) {
    // Castle flag is 0bxxxx
    //                  qkQK. First, check if our side can castle.
    const uint8_t castle_flag = 0b0011 * (turn_color == pieces::white) + 0b1100 * (turn_color == pieces::black);
    if ((castle_flag & castle) == 0)
        return 0;
    const uint8_t kingside = 0b0101;
    const uint8_t queenside = 0b1010;
    const uint64_t kingside_atk_board =  // No pieces may attack these positions
        0b0111000000000000000000000000000000000000000000000000000000000000 * (turn_color == pieces::black) +
        0b0000000000000000000000000000000000000000000000000000000001110000 * (turn_color == pieces::white);
    const uint64_t kingside_move_board =  // No pieces may block these positions
        0b0110000000000000000000000000000000000000000000000000000000000000 * (turn_color == pieces::black) +
        0b0000000000000000000000000000000000000000000000000000000001100000 * (turn_color == pieces::white);
    const uint64_t queenside_move_board =  // No pieces may block these positions
        0b0000111000000000000000000000000000000000000000000000000000000000 * (turn_color == pieces::black) +
        0b0000000000000000000000000000000000000000000000000000000000001110 * (turn_color == pieces::white);
    const uint64_t queenside_atk_board =  // No pieces may atk these positions
        0b0001110000000000000000000000000000000000000000000000000000000000 * (turn_color == pieces::black) +
        0b0000000000000000000000000000000000000000000000000000000000011100 * (turn_color == pieces::white);
    const uint64_t kingside_to =  // Where king moves kingside
        0b0100000000000000000000000000000000000000000000000000000000000000 * (turn_color == pieces::black) +
        0b0000000000000000000000000000000000000000000000000000000001000000 * (turn_color == pieces::white);
    const uint64_t queenside_to =  // Where king moves queenside
        0b0000010000000000000000000000000000000000000000000000000000000000 * (turn_color == pieces::black) +
        0b0000000000000000000000000000000000000000000000000000000000000100 * (turn_color == pieces::white);
    // Now we know we can castle. Get castle moves as branchless execution

    uint64_t castle_moves =
        ((castle_flag & kingside & castle) > 0 && ((kingside_atk_board & enemy_atk_bb) == 0) && ((kingside_move_board & all_bb) == 0)) * kingside_to +
        ((castle_flag & queenside & castle) > 0 && ((queenside_atk_board & enemy_atk_bb) == 0) && ((queenside_move_board & all_bb) == 0)) * queenside_to;
    return castle_moves;
}
}  // namespace movegen
