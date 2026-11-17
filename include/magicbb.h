#ifndef MAGIC_H
#define MAGIC_H
#include <cstint>

namespace magic{

template <class T, size_t N, T (*F)(int)> constexpr std::array<T, N> generate_simple_move_table() {
    std::array<T, N> table{};
    for (size_t i = 0; i < N; ++i) {
        // The compile-time loop populates the array
        table[i] = F(i);
    }
    return table;
}

    constexpr uint64_t occupancy_bits_rook(uint8_t sq){

    };

constexpr std::array<uint64_t, 64> rook_occupancy_table
    generate_simple_move_table<uint64_t, 64, occupancy_bits_rook>();
};











#endif