// Copyright 2025 Filip Agert
#ifndef MOVEGEN_H
#define MOVEGEN_H
#include <bitboard.h>
#include <iostream>
#include <notation_interface.h>
using namespace dirs;
using namespace masks;
namespace magic {
/*
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
 * @brief Number of bits in the rook occupancy mask
 *
 * @return Number of bits
 */
constexpr int rook_num_occ_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10,
    10, 11, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10,
    10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11, 12, 11, 11, 11, 11, 11, 11, 12};  // Target
                                                                                      // number of
//                  55  56                                      // bits to generate

/**
 * @brief Number of bits in the bishop occupancy mask
 *
 * @return Bits
 */
constexpr int bishop_num_occ_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 7, 7, 7, 7,
    5, 5, 5, 5, 7, 9, 9, 7, 5, 5, 5, 5, 7, 9, 9, 7, 5, 5, 5, 5, 7, 7,
    7, 7, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 6};  // target number of
/**
 * @brief Number of bits used in the rook hash. (Some are denser than the rook_num_occ_bits)
 *
 */
constexpr std::array<uint8_t, 64> rook_magic_sizes_bits = {
    12, 11, 11, 11, 11, 11, 11, 12, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10,
    10, 11, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10,
    10, 10, 10, 11, 10, 9,  9,  9,  9,  9,  10, 11, 12, 11, 11, 11, 11, 11, 11, 12};  // in # bits
//                   48 49                          56
/**
 * @brief Max bits in any magic table for any square rook or bishop.
 *
 * @return [TODO:description]
 */
constexpr size_t max_bits = 12;
/**
 * @brief Number of elements in the maximum magic table
 *
 * @return [TODO:description]
 */
constexpr size_t max_size = 1 << (max_bits);  // 2^12
/**
 * @brief Sizes for each array for each of the rook magic bits.
 *
 */
constexpr std::array<size_t, 64> rook_magic_sizes = [] {
    std::array<size_t, 64> rook_magic_sizes_temp;
    for (int i = 0; i < 64; i++) {
        rook_magic_sizes_temp[i] = (1 << rook_magic_sizes_bits[i]);
    }
    return rook_magic_sizes_temp;
}();

/**
 * @brief the shift values used for each rook square. 64-#bits
 *
 */
constexpr std::array<uint8_t, 64> rook_magic_shifts = [] {  // the shift value. saves a subtraction.
    std::array<uint8_t, 64> rook_magic_shifts;
    for (int i = 0; i < 64; i++) {
        rook_magic_shifts[i] = 64 - rook_magic_sizes_bits[i];
    }
    return rook_magic_shifts;
}();
/**
 * @brief Offset caused by flattening 2D array [64][nbits] into 1D. Each square has its own offset
 * to start its indexing from.
 *
 */
constexpr std::array<size_t, 64> rook_magic_offsets = [] {
    std::array<size_t, 64> offsets;
    offsets[0] = 0;
    for (int i = 1; i < 64; i++) {
        offsets[i] = offsets[i - 1] + rook_magic_sizes[i - 1];
    }
    return offsets;
}();  // ofset for 1d flattened array
/**
 * @brief Size of rook table.
 *
 * @return [TODO:description]
 */
constexpr size_t rook_magic_table_sz = rook_magic_offsets[63] + rook_magic_sizes[63];
/**
 * @brief All rook magic numbers.
 *
 */
constexpr std::array<uint64_t, 64> rook_magics = {
    36029072008052753,   2540030258560638976, 36037730559463424,   4935967732257525760,  // 4
    36033195099553920,   72066398737662976,   144119758021722753,  972780268429517568,   // 8
    4611826757832540298, 1730578526099210434, 36310409438826760,   2308235752674689408,  // 12
    2306124505668388864, 288793403415594248,  441634255655690752,  140738721480960,      // 16
    306247523444499840,  4503874511569104,    704237734760448,     216314070029176832,   // 20
    2305984297598715904, 72198881349206529,   3179554531207876610, 2314861203595219973,  // 24
    180144295406215168,  72234069953347848,   4692751087674853376, 1801527816175550472,  // 28
    162701336927039744,  3387832828312948744, 144117404279083048,  144116571055358985,   // 32
    144258399473762592,  18049618352803840,   4616331661221568512, 126383432790974506,   // 36
    9151252466239488,    2305983763890315776, 2266128965435920,    577305728096666372,   // 40
    36028934604734464,   22518135580016656,   40567585314504768,   589971585546354816,   // 44
    290271136874624,     3941749252751496,    72357236743340040,   4645437843832836,     // 48
    4035224604685392384, 5440347568166424064, 3728980139268581888, 351280590542771712,   // 52
    7755197639194824192, 155374144193791488,  360429197304463488,  35463555711488,       // 56
    10698321205149826,   81346406815903874,   4683770138187567365, 1153203598060947461,  // 60
    9288683101620225,    5066601137704962,    11259592043071492,   694842971333599362};  // 64

/**
 * @brief For a given mask, gets an array with the location of the ones in the mask.
 *
 * @param[[TODO:direction]] mask input mask.
 * @return array containing locations of ones in the mask.
 */
constexpr std::array<uint8_t, max_bits> mask_one_locs(uint64_t mask) {
    std::array<uint8_t, max_bits> one_locs;  // Initialize to disallowed value.
    uint64_t mask_reduced = mask;
    uint8_t idx = 0;
    uint8_t i = 0;
    while (mask_reduced > 0) {
        uint8_t lsb = BitBoard::lsb(mask_reduced);
        mask_reduced = (mask_reduced >> lsb) & ~1;
        idx += lsb;
        one_locs[i] = idx;
        i++;
    }
    return one_locs;
}
/**
 * @brief generates a binary number projected onto the mask. I.e. if the mask is
 * 1010
 * The number "0" corresponds to 0000
 * The number "1" corresponds to 0010
 * And the number 2 is instead   1010.
 *
 * @param[in] occ_mask mask of interest
 * @param[in] m number of bits hi in occ mask
 * @return array of possible occupancy variations to consider.
 */
constexpr std::array<uint64_t, max_size> gen_occ_variation(uint64_t occ_mask, int m) {
    // 1: Find binary rep of n
    // 2: For each 1 in n, starting from the right:
    //    Put it on each 1 in occ_mask, in the correct order.
    int num = 1 << m;  // number of occ boards to generate
    std::array<uint8_t, max_bits> mask_one_locations = mask_one_locs(occ_mask);
    std::array<uint64_t, max_size> occ_vars;

    for (int n = 0; n < num; n++) {
        int nreduced = n;
        int hi_bit = 0;
        uint64_t out_nbr = 0;
        while (nreduced > 0) {
            uint8_t hibit_temp = BitBoard::lsb(nreduced);  // Extract LSB loc.
            hi_bit += hibit_temp;
            nreduced = (nreduced >> hibit_temp) & ~1;  // Clear LSB
            // hi_bit, m, is then the location of the current high bit m in the number n. This
            // should be put on the mth 1 in occ_mask. How to find the location of the mth one in
            // occ_mask? Do the same.
            uint8_t mask_one_loc = mask_one_locations[hi_bit];
            out_nbr |= BitBoard::one_high(mask_one_loc);
        }
        occ_vars[n] = out_nbr;
    }
    return occ_vars;
}
/**
 * @brief Slow way of computing rook attack squares by casting a ray
 *
 * @param[in] sq Square of rook
 * @param[in] occ occupancy bitboard for all pieces on the board
 * @return hit: All squares the rook can attack.
 */
constexpr uint64_t rook_atk_bb_helper(int sq, const uint64_t occ) {
    uint64_t rook_bb = BitBoard::one_high(sq);
    uint64_t hit = ray(rook_bb, N, occ);
    hit |= ray(rook_bb, S, occ);
    hit |= ray(rook_bb, E, occ);
    hit |= ray(rook_bb, W, occ);
    return hit;
}
constexpr uint64_t bishop_atk_bb_helper(int sq, const uint64_t occ) {
    /**
     * @brief Slow way of computing bishop attack squares by casting a ray
     *
     * @param[in] sq Square of bishop
     * @param[in] occ occupancy bitboard for all pieces on the board
     * @return hit: All squares the bishop can attack.
     */
    uint64_t bishop_bb = BitBoard::one_high(sq);
    uint64_t hit = ray(bishop_bb, NE, occ);
    hit |= ray(bishop_bb, SE, occ);
    hit |= ray(bishop_bb, SW, occ);
    hit |= ray(bishop_bb, NW, occ);
    return hit;
}
/**
 * @brief Generate all possible attack bitboards for all possible occupancy tables
 *
 * @param[in] occ_vars occupancy bb for relevant mask
 * @param[in] sq square of rook/bishop
 * @param[in] rook is rook or bishop?
 */
constexpr std::array<uint64_t, max_size> compute_atk_bbs(std::array<uint64_t, max_size> occ_vars,
                                                         int sq, bool rook) {
    int m = rook ? rook_num_occ_bits[sq] : bishop_num_occ_bits[sq];
    int num = 1 << m;  // number of occ boards to generate.
    std::array<uint64_t, max_size> atk_bbs;
    for (int n = 0; n < num; n++) {
        atk_bbs[n] =
            rook ? rook_atk_bb_helper(sq, occ_vars[n]) : bishop_atk_bb_helper(sq, occ_vars[n]);
    }
    return atk_bbs;
}
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
    uint64_t aroundmask = masks::top * (row != 7) | masks::bottom * (row != 0) |
                          masks::left * (col != 0) | masks::right * (col != 7);
    uint64_t occbits =
        (masks::col(col) | masks::row(row)) & (~(aroundmask | BitBoard::one_high(sq)));
    return occbits;
}

/**
 * @brief Mask of the relevant bits in a rook magic bitboard calculation.
 *
 */
constexpr std::array<uint64_t, 64> rook_occupancy_table = [] {
    std::array<uint64_t, 64> arr;
    for (int sq = 0; sq < 64; sq++) {
        arr[sq] = occupancy_bits_rook(sq);
    }
    return arr;
}();  // Will be used as a key.

/**
 * @brief Mask calculator for the relevant bits in a bishop magic bitboard calculation.
 *
 * @param[in] sq square of bishop
 * @return bitboard containing bits where the occupancy matters for this square.
 */
constexpr uint64_t occupancy_bits_bishop(uint8_t sq) {
    uint8_t row = NotationInterface::row(sq);
    uint8_t col = NotationInterface::col(sq);
    uint64_t aroundmask = masks::top * (row != 7) | masks::bottom * (row != 0) |
                          masks::left * (col != 0) | masks::right * (col != 7);
    return bishop_atk_bb_helper(sq, 0) & ~(aroundmask | BitBoard::one_high(sq));
}
constexpr std::array<uint64_t, 64> bishop_occupancy_table = [] {
    std::array<uint64_t, 64> arr;
    for (int sq = 0; sq < 64; sq++) {
        arr[sq] = occupancy_bits_bishop(sq);
    }
    return arr;
}();  // Will be used as a key for magic bitboard.
/**
 * @brief Gets the key for the rook magic bitboard table.
 *
 * @param[in] sq Square of rook
 * @param[in] occ occupancy bitboard (all pieces)
 * @return the key to lookup the magic bitboard with.
 */
constexpr uint64_t get_rook_key(uint8_t sq, uint64_t occ) {
    uint8_t shift = rook_magic_shifts[sq];
    uint64_t magic = rook_magics[sq];
    uint64_t occmask = rook_occupancy_table[sq];
    return (((occmask & occ) * magic) >> shift);
}
/**
 * @brief Gets the index to access the flattened rook bitboard array with.
 *
 * @param[in] sq square of rook
 * @param[in] occ occupancy bitboard (can be unmasked.)
 * @return index to access array rook_magic_bitboards with.
 */
constexpr int get_rook_magic_idx(uint8_t sq, uint64_t occ) {
    return get_rook_key(sq, occ) + rook_magic_offsets[sq];
}
/**
 * @brief Table of rook magic bitboards. Indexed by the function get_rook_magic_idx
 *
 */
constexpr std::array<uint64_t, rook_magic_table_sz> rook_magic_bitboards =
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
/**
 * @brief Gets all the squares the rook can reach from the given position given an occupancy of the
 * board stored in the occ bitboard. Needs to be masked with friendly pieces to not capture them.
 *
 * @param[in] sq Square of rook
 * @param[in] occ Occupancy bitboard for hess board
 * @return All attackable squares for the rook at sq.
 */
constexpr uint64_t get_rook_atk_bb(uint8_t sq, uint64_t occ) {
    return rook_magic_bitboards[get_rook_magic_idx(sq, occ)];
}
}  // namespace magic

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
    uint64_t hit = magic::ray(king_bb, N, 0, 1);
    hit |= magic::ray(king_bb, NE, 0, 1);
    hit |= magic::ray(king_bb, E, 0, 1);
    hit |= magic::ray(king_bb, SE, 0, 1);
    hit |= magic::ray(king_bb, S, 0, 1);
    hit |= magic::ray(king_bb, SW, 0, 1);
    hit |= magic::ray(king_bb, W, 0, 1);
    hit |= magic::ray(king_bb, NW, 0, 1);
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
    uint64_t hit = magic::ray(bishop_bb, NE, occ);
    hit |= magic::ray(bishop_bb, SE, occ);
    hit |= magic::ray(bishop_bb, SW, occ);
    hit |= magic::ray(bishop_bb, NW, occ);
    return hit;
}

constexpr std::array<uint64_t, 64> king_attack_table =  // 512 bytes
    generate_simple_move_table_uint8_t<uint64_t, 64, king_atk_sq>();
constexpr std::array<uint64_t, 64> knight_attack_table =  // 512 bytes
    generate_simple_move_table_uint8_t<uint64_t, 64, knight_atk_sq>();

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
constexpr uint64_t rook_moves_sq(const uint8_t rook_loc, const uint64_t friendly_bb,
                                 const uint64_t enemy_bb) {
    return magic::get_rook_atk_bb(rook_loc, friendly_bb | enemy_bb) & ~(friendly_bb);
}
/**
 * @brief Generate all squares that all rooks atk FIX: implement this.
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
constexpr uint64_t rook_atk(const uint8_t sq, const uint64_t occ) {
    return magic::get_rook_atk_bb(sq, occ);
}
uint64_t bishop_moves(const uint64_t bishop_bb, const uint64_t friendly_bb,
                      const uint64_t enemy_bb);
uint64_t queen_moves(const uint64_t queen_bb, const uint64_t friendly_bb, const uint64_t enemy_bb);
uint64_t pawn_moves(const uint64_t pawn_bb, const uint64_t friendly_bb, const uint64_t enemy_bb,
                    const uint64_t ep_bb, const uint8_t pawn_color);
uint64_t pawn_attack_moves(const uint64_t pawn_bb, const uint64_t enemy_bb, const uint64_t ep_bb,
                           const uint8_t pawn_col);

}  // namespace movegen

#endif  // MOVEGEN_H
