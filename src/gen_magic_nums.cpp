
#include <algorithm>
#include <bitboard.h>
#include <cstdlib>
#include <iostream>
#include <movegen.h>
#include <utility>
#include <vector>
using namespace movegen;
using namespace magic;
// bits to generate.
//

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
size_t get_key(uint64_t occ, uint64_t magic, uint8_t bits) { return (occ * magic) >> (64 - bits); }

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

std::optional<std::pair<uint64_t, size_t>> find_magic_nbr(int maxiter, int sq, int bits, bool rook,
                                                          std::array<uint64_t, max_size> &occ_bbs,
                                                          std::array<uint64_t, max_size> &atk_bbs) {
    int m = rook ? rook_num_occ_bits[sq] : bishop_num_occ_bits[sq];
    int arrsz = 1 << m;  // size of mask and atk arrays
    int hashsz = 1 << bits;
    std::vector<uint64_t> hash_table(hashsz);
    bool found = false;
    uint64_t magic = 0;
    int niter = 0;
    uint64_t occmask = rook ? occupancy_bits_rook(sq) : occupancy_bits_bishop(sq);
    int magic_minval = get_magic_minval(occmask, bits);
    size_t key_maxval = 0;
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
        key_maxval = 0;
        for (int n = 0; n < arrsz; n++) {
            size_t key = get_key(occ_bbs[n], magic, bits);
            key_maxval = std::max(key, key_maxval);
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
    if (found) {
        std::pair<uint64_t, size_t> out = {magic, key_maxval};
        return out;
    } else {
        return {};
    }
}

void find_dense(bool rook) {
    std::array<std::pair<uint64_t, uint8_t>, 64> magics;

    for (int sq = 48; sq < 53; sq++) {
        int target_bits = rook ? rook_num_occ_bits[sq] : bishop_num_occ_bits[sq];
        target_bits -= 1;
        std::pair<uint64_t, uint8_t> init = {0, target_bits};
        magics[sq] = init;
        uint64_t occ_mask = rook ? rook_occupancy_table[sq] : bishop_occupancy_table[sq];
        int m = rook ? rook_num_occ_bits[sq] : bishop_num_occ_bits[sq];
        std::array<uint64_t, max_size> occ_bbs = gen_occ_variation(occ_mask, m);  // will be size
        std::array<uint64_t, max_size> atk_bbs = compute_atk_bbs(occ_bbs, sq, rook);
        int maxiter = 10000000;
        std::optional<std::pair<uint64_t, size_t>> magic_candidate;
        magic_candidate = find_magic_nbr(maxiter, sq, target_bits, rook, occ_bbs, atk_bbs);

        if (magic_candidate) {
            magics[sq] = {magic_candidate.value().first, target_bits};
            std::cout << "found magic\n " << sq << " " << target_bits << " "
                      << magic_candidate.value().first << "\n";
        } else {
            std::cout << "did not find magic: " << sq << "\r";
        }
        fflush(stdout);
    }
    std::cout << "Square   foundbits     magic\n";
    for (int sq = 0; sq < 64; sq++) {
        std::cout << sq << " " << magics[sq].second << " " << magics[sq].first << "\n";
    }
}
/**
 * @brief For finding magic numbers with the normal amount of bits but a lower hash range than
 * usual.
 *
 * @param[[TODO:direction]] rook [TODO:description]
 */
void find_lower_range(bool rook) {
    std::array<std::pair<uint64_t, size_t>, 64> magics;
    std::array<std::array<uint64_t, max_size>, 64> occs_bbs_sq;
    std::array<std::array<uint64_t, max_size>, 64> atk_bbs_sq;
    for (int sq = 0; sq < 64; sq++) {
        int target_bits = rook ? rook_num_occ_bits[sq] : bishop_num_occ_bits[sq];
        uint64_t occ_mask = rook ? rook_occupancy_table[sq] : bishop_occupancy_table[sq];
        int m = rook ? rook_num_occ_bits[sq] : bishop_num_occ_bits[sq];
        std::array<uint64_t, max_size> occ_bbs = gen_occ_variation(occ_mask, m);  // will be size
        std::array<uint64_t, max_size> atk_bbs = compute_atk_bbs(occ_bbs, sq, rook);
        occs_bbs_sq[sq] = occ_bbs;
        atk_bbs_sq[sq] = atk_bbs;
        magics[sq] = {0, 10000000};
        int maxiter = 100000;
        int outer = 2000;
        for (int i = 0; i < outer; i++) {
            std::optional<std::pair<uint64_t, size_t>> magic_candidate =
                find_magic_nbr(maxiter, sq, m, rook, occ_bbs, atk_bbs);

            if (magic_candidate) {
                int upper = magic_candidate.value().second;
                if (upper < magics[sq].second) {
                    magics[sq] = magic_candidate.value();
                    if (upper + 1 != int(1 << target_bits)) {
                        std::cout << "found magic with better hash range: " << upper + 1
                                  << " when the standard value is: " << int(1 << target_bits)
                                  << "\r";
                    }
                }
            }
            fflush(stdout);
        }
    }
    std::cout << "Square   range     standard         magic\n";
    for (int sq = 0; sq < 64; sq++) {
        int target_bits = rook ? rook_num_occ_bits[sq] : bishop_num_occ_bits[sq];
        std::cout << sq << " " << magics[sq].second + 1 << " " << int(1 << target_bits) << " "
                  << magics[sq].first << "\n";
    }
}

void find_sparse(bool rook) {
    std::array<std::pair<uint64_t, uint8_t>, 64> magics;
    std::array<std::array<uint64_t, max_size>, 64> occs_bbs_sq;
    std::array<std::array<uint64_t, max_size>, 64> atk_bbs_sq;
    for (int sq = 0; sq < 64; sq++) {
        int target_bits = rook ? rook_num_occ_bits[sq] : bishop_num_occ_bits[sq];
        uint64_t occ_mask = rook ? rook_occupancy_table[sq] : bishop_occupancy_table[sq];
        int m = rook ? rook_num_occ_bits[sq] : bishop_num_occ_bits[sq];
        std::array<uint64_t, max_size> occ_bbs = gen_occ_variation(occ_mask, m);  // will be size
        std::array<uint64_t, max_size> atk_bbs = compute_atk_bbs(occ_bbs, sq, rook);
        occs_bbs_sq[sq] = occ_bbs;
        atk_bbs_sq[sq] = atk_bbs;
        int maxiter = 10000;

        std::optional<std::pair<uint64_t, size_t>> magic_candidate;
        while (!magic_candidate) {
            magic_candidate = find_magic_nbr(maxiter, sq, target_bits, rook, occ_bbs, atk_bbs);
        }
        magics[sq] = {magic_candidate.value().first, target_bits};
        std::cout << "found magic: " << sq << "\r";
        fflush(stdout);
    }
    std::cout << "Square   foundbits     magic\n";
    for (int sq = 0; sq < 64; sq++) {
        int target_bits = rook ? rook_num_occ_bits[sq] : bishop_num_occ_bits[sq];
        std::cout << sq << " " << target_bits << " " << magics[sq].first << "\n";
    }
}
int main() {

    find_lower_range(true);
    return 0;
}
