
#include <movegen.h>
int RBits[64] = {12, 11, 11, 11, 11, 11, 11, 12, 11, 10, 10, 10, 10, 10, 10, 11, 11,
                 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10,
                 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10,
                 10, 10, 10, 10, 11, 12, 11, 11, 11, 11, 11, 11, 12};  // Target number of bits to
                                                                       // generate

int BBits[64] = {6, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 7, 7, 7, 7,
                 5, 5, 5, 5, 7, 9, 9, 7, 5, 5, 5, 5, 7, 9, 9, 7, 5, 5, 5, 5, 7, 7,
                 7, 7, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 6};  // target number of
                                                                               // bits to generate.

uint64_t find_magic_nbr(int sq, int bits, bool rook) {
    uint64_t occ_mask =
        rook ? movegen::rook_occupancy_table[sq] : movegen::bishop_occupancy_table[sq];

    // Goal is to find a number which has the property:
    // When multiplied by mask, the relevant n bits in the mask should hash to the m highest bits in
    // a 64 bit number. Then shift by 64-m to obtain a hash value. Now, we need to check that for
    // all combinations in the n bit number, they are totally unique, i.e. there are no collisions
    // when going n->m. Ideally we want n=m, or perhaps even lower. By no collisions, we dont mean
    // several combinations of the masks n collide on the same hash in the m bit number. We rather
    // mean that if masks n collide, they must also generate the same rook move table. We want
    // collisions if they generate the same table.
}
int main() {}
