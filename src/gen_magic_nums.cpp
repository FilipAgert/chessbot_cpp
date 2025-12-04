
#include <bitboard.h>
#include <cstdlib>
#include <iostream>
#include <movegen.h>
#include <utility>
#include <vector>
constexpr int RBits[64] = {12, 11, 11, 11, 11, 11, 11, 12, 11, 10, 10, 10, 10, 10, 10, 11, 11,
                           10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10,
                           10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10,
                           10, 10, 10, 10, 11, 12, 11, 11, 11, 11, 11, 11, 12};  // Target number of
                                                                                 // bits to generate

int BBits[64] = {6, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 7, 7, 7, 7,
                 5, 5, 5, 5, 7, 9, 9, 7, 5, 5, 5, 5, 7, 9, 9, 7, 5, 5, 5, 5, 7, 7,
                 7, 7, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 6};  // target number of
                                                                               // bits to generate.
//
constexpr size_t max_bits = 12;
constexpr size_t max_size = 1 << (max_bits);  // 2^12
/**
 * @brief For a given mask, gets an array with the location of the ones in the mask.
 *
 * @param[[TODO:direction]] mask [TODO:description]
 * @return array containing ones in the mask.
 */
std::array<uint8_t, max_bits> mask_one_locs(uint64_t mask) {
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
std::array<uint64_t, max_size> gen_occ_variation(uint64_t occ_mask, int m) {
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
 * @brief Generate all possible attack bitboards for all possible occupancy tables
 *
 * @param[in] occ_vars occupancy bb for relevant mask
 * @param[in] sq square of rook/bishop
 * @param[in] rook is rook or bishop?
 */
std::array<uint64_t, max_size> compute_atk_bbs(std::array<uint64_t, max_size> occ_vars, int sq,
                                               bool rook) {
    int m = rook ? RBits[sq] : BBits[sq];
    int num = 1 << m;  // number of occ boards to generate.
    std::array<uint64_t, max_size> atk_bbs;
    uint64_t sq_bb = BitBoard::one_high(sq);
    for (int n = 0; n < num; n++) {
        atk_bbs[n] =
            rook ? movegen::rook_atk(sq, occ_vars[n]) : movegen::bishop_atk(sq_bb, occ_vars[n]);
    }
    return atk_bbs;
}

uint64_t rand_uint64_t() {
    int r1 = rand();
    int r2 = rand();
    uint64_t randomuint64_t = (static_cast<uint64_t>(r1) << 32) | static_cast<uint64_t>(r2);
    return randomuint64_t;
}
uint64_t get_next_magic() {
    uint64_t r1 = rand_uint64_t();
    uint64_t r2 = rand_uint64_t();
    uint64_t r4 = rand_uint64_t();
    return r1 & r2 & r4;
    //      r4;  // Generate a random number with 1/16th of the bits set high on average.
}
/**
 * @brief Gets magic bitboard key
 *
 * @param[in] occ Occupancy bitboard masked by relevant squares
 * @param[in] magic Magic number for this square and piece type
 * @param[in] bits number of bits in the hash table
 * @return out key for bitboard
 */
uint64_t get_key(uint64_t occ, uint64_t magic, uint8_t bits) {
    return (occ * magic) >> (64 - bits);
}

/**
 * @brief Gets the minimal value the magic number must be.
 *
 * @param[[TODO:direction]] occ [TODO:description]
 * @param[[TODO:direction]] shift [TODO:description]
 * @return exponent of minval >= 2**exp
 */
int get_magic_minval(uint64_t occmask, int shift) {
    int lsb = BitBoard::lsb(occmask);
    int exp = 64 - shift - lsb;
    return exp;
}

std::pair<uint64_t, bool> find_magic_nbr(int maxiter, int sq, int bits, bool rook,
                                         std::array<uint64_t, max_size> &occ_bbs,
                                         std::array<uint64_t, max_size> &atk_bbs) {
    int m = rook ? RBits[sq] : BBits[sq];
    int arrsz = 1 << m;  // size of mask and atk arrays
    int hashsz = 1 << bits;
    std::vector<uint64_t> hash_table(hashsz);
    bool found = false;
    uint64_t magic = 0;
    int niter = 0;
    uint64_t occmask = rook ? movegen::occupancy_bits_rook(sq) : movegen::occupancy_bits_bishop(sq);
    int magic_minval = get_magic_minval(occmask, bits);

    while (!found && niter < maxiter) {
        magic = 0;
        niter++;
        std::fill(hash_table.begin(), hash_table.end(),
                  0xFFFFFFFFFFFFFFFF);  // The atk tables can never be zero and thus this is fine to
                                        // initialize.

        while ((magic >> magic_minval) ==
               0ULL)  // assert that the magic number must be greater than the minimum value.
            magic = (get_next_magic());

        bool is_magic = true;
        for (int n = 0; n < arrsz; n++) {
            uint64_t key = get_key(occ_bbs[n], magic, bits);
            uint64_t atk_bb = atk_bbs[n];
            if (hash_table[key] == 0xFFFFFFFFFFFFFFFF) {
                hash_table[key] = atk_bb;
            } else if (hash_table[key] != atk_bb) {  // Collision. Need to restart.
                is_magic = false;
                break;
            }
        }
        if (is_magic) {
            found = true;
            if (magic == 0) {
                std::cout << "found magic = 0. \n";
            }
            break;
        }
    }

    // Goal is to find a number which has the property:
    // When multiplied by mask, the relevant n bits in the mask should hash to the m highest bits in
    // a 64 bit number. Then shift by 64-m to obtain a hash value. Now, we need to check that for
    // all combinations in the n bit number, they are totally unique, i.e. there are no collisions
    // when going n->m. Ideally we want n=m, or perhaps even lower. By no collisions, we dont mean
    // several combinations of the masks n collide on the same hash in the m bit number. We rather
    // mean that if masks n collide, they must also generate the same rook move table. We want
    // collisions if they generate the same table.
    std::pair<uint64_t, bool> out = {magic, found};
    return out;
}

void find_dense(bool rook) {
    std::array<std::pair<uint64_t, uint8_t>, 64> magics;

    for (int sq = 63; sq < 64; sq++) {
        int target_bits = rook ? RBits[sq] : BBits[sq];
        target_bits -= 1;
        std::pair<uint64_t, uint8_t> init = {0, target_bits};
        magics[sq] = init;
        uint64_t occ_mask =
            rook ? movegen::rook_occupancy_table[sq] : movegen::bishop_occupancy_table[sq];
        int m = rook ? RBits[sq] : BBits[sq];
        std::array<uint64_t, max_size> occ_bbs = gen_occ_variation(occ_mask, m);  // will be size
        std::array<uint64_t, max_size> atk_bbs = compute_atk_bbs(occ_bbs, sq, rook);
        int maxiter = 1000000000;
        bool found = false;
        int outeriter = 50;
        std::pair<uint64_t, bool> magic_candidate;
        magic_candidate = find_magic_nbr(maxiter, sq, target_bits, rook, occ_bbs, atk_bbs);
        found = magic_candidate.second;
        if (found) {
            magics[sq] = {magic_candidate.first, target_bits};
            std::cout << "found magic\n " << sq << " " << target_bits << " "
                      << magic_candidate.first << "\n";
        } else {
            std::cout << "did not find magic: " << sq << "\r";
        }
        fflush(stdout);
    }
    std::cout << "Square   foundbits     magic\n";
    for (int sq = 0; sq < 64; sq++) {
        int target_bits = rook ? RBits[sq] : BBits[sq];
        std::cout << sq << " " << target_bits << " " << magics[sq].first << "\n";
    }
}

void find_sparse(bool rook) {
    std::array<std::pair<uint64_t, uint8_t>, 64> magics;
    std::array<std::array<uint64_t, max_size>, 64> occs_bbs_sq;
    std::array<std::array<uint64_t, max_size>, 64> atk_bbs_sq;
    for (int sq = 0; sq < 64; sq++) {
        int target_bits = rook ? RBits[sq] : BBits[sq];
        uint64_t occ_mask =
            rook ? movegen::rook_occupancy_table[sq] : movegen::bishop_occupancy_table[sq];
        int m = rook ? RBits[sq] : BBits[sq];
        std::array<uint64_t, max_size> occ_bbs = gen_occ_variation(occ_mask, m);  // will be size
        std::array<uint64_t, max_size> atk_bbs = compute_atk_bbs(occ_bbs, sq, rook);
        occs_bbs_sq[sq] = occ_bbs;
        atk_bbs_sq[sq] = atk_bbs;
        int maxiter = 10000;
        bool found = false;

        std::pair<uint64_t, bool> magic_candidate;
        while (!found) {
            magic_candidate = find_magic_nbr(maxiter, sq, target_bits, rook, occ_bbs, atk_bbs);
            found = magic_candidate.second;
        }
        magics[sq] = {magic_candidate.first, target_bits};
        std::cout << "found magic: " << sq << "\r";
        fflush(stdout);
    }
    std::cout << "Square   foundbits     magic\n";
    for (int sq = 0; sq < 64; sq++) {
        int target_bits = rook ? RBits[sq] : BBits[sq];
        std::cout << sq << " " << target_bits << " " << magics[sq].first << "\n";
    }
}
int main() {

    find_sparse(true);
    return 0;
}
