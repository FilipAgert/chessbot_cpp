// Copyright 2025 Filip Agert
#include "core/bitboard.h"
#include "core/notation_interface.h"
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
        ss << rank + 1 << " |"; // Rank label
        for (int file = 0; file < 8; ++file) {
            int index = rank * 8 + file;
            // Check if the bit at the current square index is set
            if ((bb >> index) & 1) {
                ss << " 1"; // High bit
            } else {
                ss << " -"; // Low bit
            }
        }
        ss << " |\n";
    }
    ss << "   -----------------\n";

    return ss.str();
}

void print_full(uint64_t bb) { std::cout << BitBoard::to_string_bb(bb) << "\n"; }
} // namespace BitBoard
namespace masks {
alignas(64) const std::array<std::array<BB, 64>, 64> rect_lookup = [] {
    std::array<std::array<BB, 64>, 64> arr;
    arr.fill({0ULL});
    for (int i = 0; i < 64; i++) {
        // Vertical:
        int rowinit = NotationInterface::row(i);
        int colinit = NotationInterface::col(i);
        BB startsq = BitBoard::one_high(i);
        for (int r = 0; r < 8; r++) {
            int j = NotationInterface::idx(r, colinit);
            BB to = BitBoard::one_high(j);
            if (i == j)
                continue;
            // Fill all between i and j.
            // If positive, move up.
            int dir = j > i ? dirs::N : dirs::S;
            BB mask = ray(startsq, dir, to);
            arr[i][j] = mask & ~startsq;
        }

        for (int c = 0; c < 8; c++) {
            int j = NotationInterface::idx(rowinit, c);
            BB to = BitBoard::one_high(j);
            // Fill all between i and j.
            // If positive, move up.
            if (i == j)
                continue;

            int dir = c > colinit ? dirs::E : dirs::W;
            BB mask = ray(startsq, dir, to);
            arr[i][j] = mask & ~startsq;
        }

        {
            // NE/SW diagonal.
            // If row > col : COL = 0.
            int c_start = (rowinit >= colinit) ? 0 : colinit - rowinit;
            int r_start = (rowinit >= colinit) ? rowinit - colinit : 0;

            for (int r = r_start, c = c_start; r < 8 && c < 8; r++, c++) {
                int j = NotationInterface::idx(r, c);

                if (i == j)
                    continue;

                BB to = BitBoard::one_high(j);

                int dir = j > i ? dirs::NE : dirs::SW;

                BB mask = ray(startsq, dir, to);
                arr[i][j] = mask & ~startsq;
            }
        }

        {
            // NW/SE diagonal.
            // row + column = constant.
            int c_start = (rowinit + colinit < 7) ? colinit + rowinit : 7;
            int r_start = (rowinit + colinit < 7) ? 0 : rowinit + colinit - 7;

            for (int r = r_start, c = c_start; r < 8 && c >= 0; r++, c--) {
                int j = NotationInterface::idx(r, c);

                if (i == j)
                    continue;

                BB to = BitBoard::one_high(j);

                int dir = j > i ? dirs::NW : dirs::SE;

                BB mask = ray(startsq, dir, to);
                arr[i][j] = mask & ~startsq;
            }
        }
    }
    return arr;
}();
alignas(64) extern const std::array<BB, 128> NS_wall_lookup = []() {
    std::array<BB, 128> arr;
    auto safe_shift = [](BB bb, int dir) constexpr -> BB { return (dir >= 0) ? (bb << dir) : (bb >> -dir); };
    for (size_t i = 0; i < 64; i++) {
        BB sq = BitBoard::one_high(i);
        for (size_t c = 0; c <= 1; c++) {
            int dir_ns = (c == 0) ? dirs::S : dirs::N;
            int dir_w = (c == 0) ? dirs::SW : dirs::NW;
            int dir_e = (c == 0) ? dirs::SE : dirs::NE;

            BB out_ns = safe_shift(sq & ~edge_mask(dir_ns), dir_ns);
            BB out_w = safe_shift(sq & ~edge_mask(dir_w), dir_w);
            BB out_e = safe_shift(sq & ~edge_mask(dir_e), dir_e);

            arr[i + 64 * c] = out_ns | out_w | out_e;
        }
    }
    return arr;
}();
alignas(64) extern const std::array<BB, 64> three_x_three = []() {
    std::array<BB, 64> arr;
    for (size_t i = 0; i < 64; i++) {
        BB out = BitBoard::one_high(i);
        out |= NS_wall_lookup[i] | NS_wall_lookup[i + 64];
        int col = NotationInterface::col(i);
        int row = NotationInterface::row(i);

        int uprow = std::min(row + 1, 7);
        int downrow = std::max(0, row - 1);
        int leftcol = std::max(col - 1, 0);
        int rightcol = std::min(col + 1, 7);
        // left
        out |= BitBoard::one_high(NotationInterface::idx(uprow, leftcol));
        out |= BitBoard::one_high(NotationInterface::idx(row, leftcol));
        out |= BitBoard::one_high(NotationInterface::idx(downrow, leftcol));

        // right
        out |= BitBoard::one_high(NotationInterface::idx(uprow, rightcol));
        out |= BitBoard::one_high(NotationInterface::idx(row, rightcol));
        out |= BitBoard::one_high(NotationInterface::idx(downrow, rightcol));

        // up
        out |= BitBoard::one_high(NotationInterface::idx(uprow, col));
        // down
        out |= BitBoard::one_high(NotationInterface::idx(downrow, col));

        arr[i] = out;
    }
    return arr;
}();
} // namespace masks