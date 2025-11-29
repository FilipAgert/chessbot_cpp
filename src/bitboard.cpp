// Copyright 2025 Filip Agert
#include <bitboard.h>
#include <iostream>
#include <sstream>
#include <string>
using namespace dirs;
using namespace masks;
namespace BitBoard {

std::string to_string_bb(uint64_t bb) {
    std::stringstream ss;

    // File labels
    ss << "\n    a b c d e f g h\n";
    ss << "   -----------------\n";

    // Iterate ranks from 8 (index 7) down to 1 (index 0)
    for (int rank = 7; rank >= 0; --rank) {
        ss << rank + 1 << " |";  // Rank label
        for (int file = 0; file < 8; ++file) {
            int index = rank * 8 + file;
            // Check if the bit at the current square index is set
            if ((bb >> index) & 1) {
                ss << " 1";  // High bit
            } else {
                ss << " -";  // Low bit
            }
        }
        ss << " |\n";
    }
    ss << "   -----------------\n";

    return ss.str();
}

void print_full(uint64_t bb) { std::cout << BitBoard::to_string_bb(bb) << "\n"; }

}  // namespace BitBoard

namespace masks {

uint64_t edge_mask(int dir) {
    uint64_t outmask = 0;
    switch (dir) {
    case E:
        outmask = right;
        break;
    case W:
        outmask = left;
        break;
    case S:
        outmask = bottom;
        break;
    case N:
        outmask = top;
        break;
    case NE:
        outmask = top | right;
        break;
    case NW:
        outmask = top | left;
        break;
    case SE:
        outmask = bottom | right;
        break;
    case SW:
        outmask = bottom | left;
        break;
    default:
        std::cerr << "err: wrong dir" << std::endl;
        abort();
        break;
    }
    return outmask;
}
}  // namespace masks
