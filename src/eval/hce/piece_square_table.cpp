#include "eval/hce/piece_square_table.h"
#include "core/notation_interface.h"
#include <sstream>
namespace Eval::HCE {

void south_north_mirror(std::array<int, 64> &arr) {
    // pastes south onto north, mirrored
    for (int i = 0; i < 32; i++) {
        arr[NotationInterface::idx_flip(i)] = arr[i];
    }
}
void west_east_mirror(std::array<int, 64> &arr) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 4; col++) {
            int wcol = 3 - col;
            int wsq = NotationInterface::idx(row, wcol);

            int ecol = 4 + col;
            int esq = NotationInterface::idx(row, ecol);
            arr[esq] = arr[wsq];
        }
    }
}
std::string symmetry_name(Symmetry s) {
    if (s == Symmetry::EW) {
        return "EW";
    }
    if (s == Symmetry::EWNS) {
        return "EWNS";
    }
    if (s == Symmetry::NS) {
        return "NS";
    }
    if (s == Symmetry::PIZZA) {
        return "PIZZA";
    }
    return "none";
}
// Piece square tables
int PieceSquareTable::num_params(Symmetry s, Piece_t p) const {
    if (s == Symmetry::None)
        return 64 - (p == pieces::pawn ? 16 : 0);
    if (s == Symmetry::EW || s == Symmetry::NS)
        return 32 - (p == pieces::pawn ? 8 : 0);
    if (s == Symmetry::PIZZA)
        return 10 - (p == pieces::pawn ? 4 : 0);
    return 16 - (p == pieces::pawn ? 4 : 0);
}
void PieceSquareTable::set(Symmetry s, Piece_t p, std::vector<int> params_) {
    if (params_.size() != num_params(s, p)) {
        throw std::invalid_argument("Wrong number of parameters. Expected: " + std::to_string(num_params(s, p)) +
                                    ". Received: " + std::to_string(params_.size()));
    }

    sym_ = s;
    params = params_;
    piece_ = p;
    rebuild();
}

int PieceSquareTable::num_cols(Symmetry s) {
    if (s == Symmetry::NS || s == Symmetry::None)
        return 8;
    return 4;
}

void PieceSquareTable::rebuild() {
    int np = num_params(sym_, piece_);
    int idx_offset = piece_ == pieces::pawn ? 8 : 0;
    if (sym_ == Symmetry::None) {
        for (size_t i = 0; i < np; i++) {
            white_[i + idx_offset] = params.at(i);
        }
    } else if (sym_ == Symmetry::NS) { // 32 pieces.
        for (size_t i = 0; i < np; i++) {
            white_[i + idx_offset] = params.at(i);
        }
        south_north_mirror(white_);
    } else if (sym_ == Symmetry::EW || sym_ == Symmetry::EWNS) {
        int row = piece_ == pieces::pawn ? 1 : 0;
        int col = 0;
        // go A1...D1, A2...D2 up to maxrows

        for (size_t i = 0; i < np; i++) {
            int idx = NotationInterface::idx(row, col);
            white_[idx] = params.at(i);

            if (++col >= 4) {
                col = 0;
                row++;
            }
        }
        if (sym_ == Symmetry::EWNS)
            south_north_mirror(white_);
        west_east_mirror(white_);
    } else if (sym_ == Symmetry::PIZZA) {
        int row = piece_ == pieces::pawn ? 1 : 0;
        int col = piece_ == pieces::pawn ? 1 : 0;

        for (size_t i = 0; i < np; i++) {
            int idx = NotationInterface::idx(row, col);
            white_[idx] = params.at(i);
            int mirrored = NotationInterface::idx(col, row);
            white_[mirrored] = white_[idx];
            if (++col >= 4) {
                row++;
                col = row;
            }
        }
        south_north_mirror(white_);
        west_east_mirror(white_);

    } else {
        throw std::runtime_error("Unknown symmetry");
    }

    // mirror onto black.
    for (size_t i = 0; i < white_.size(); i++) {
        int black_idx = NotationInterface::idx_flip(i);
        black_[black_idx] = white_[i];
    }
}
size_t PieceSquareTable::size() const { return params.size(); }
size_t PieceSquareTable::width() const {
    if (sym_ != Symmetry::EW && sym_ != Symmetry::EWNS && sym_ != Symmetry::PIZZA)
        return 8;
    return 4;
}
size_t PieceSquareTable::height() const {
    if (sym_ != Symmetry::NS && sym_ != Symmetry::EWNS && sym_ != Symmetry::PIZZA)
        return 8;
    return 4;
}
Symmetry PieceSquareTable::symmetry() const { return sym_; }

std::string PieceSquareTable::to_string() const {
    const size_t w = width(), h = height();

    size_t cw = 5; // inner cell width, same for every cell
    for (int v : params)
        cw = std::max(cw, std::to_string(v).size() + 2);

    std::string sep = "   +";
    for (int f = 0; f < 8; ++f)
        sep += std::string(cw, '-') + "+";
    sep += "\n";

    auto centered = [&](const std::string &s) {
        const size_t total = cw - s.size();
        const size_t left = total / 2;
        return std::string(left, ' ') + s + std::string(total - left, ' ');
    };

    std::ostringstream os;
    os << "symmetry: " << symmetry_name(sym_) << "\n";
    os << "    ";
    for (int f = 0; f < 8; ++f)
        os << centered(std::string(1, char('a' + f))) << " ";
    os << "\n" << sep;
    for (int r = 7; r >= 0; --r) {
        os << " " << (r + 1) << " |";
        for (int f = 0; f < 8; ++f) {
            int offset = piece_ == pieces::pawn ? 1 : 0;
            int idx = NotationInterface::idx(r, f);
            const bool store = ((static_cast<size_t>(f) < w) && (static_cast<size_t>(r) < (h - offset)) && (static_cast<size_t>(r) >= offset) &&
                                (sym_ == Symmetry::PIZZA ? f >= r : true));
            os << centered(store ? std::to_string(white_[idx]) : "") << "|";
        }
        os << " " << (r + 1) << "\n" << sep;
    }
    os << "    ";
    for (int f = 0; f < 8; ++f)
        os << centered(std::string(1, char('a' + f))) << " ";
    os << "\n{";
    for (size_t i = 0; i < params.size(); ++i)
        os << params[i] << ((i < params.size() - 1) ? ", " : "}\n");
    return os.str();
}

/**
 * Collection of piece square tables.
 *
 */
using PstItem = std::pair<std::vector<int>, Symmetry>;
PstItem king_pst = []() {
    Symmetry s = Symmetry::EW;
    std::vector<int> values{0,   51,  -8,  -11, 40,  38,  -12, -18, -21, -20, -31, -32, -38, -27, -42, -52,
                            -46, -25, -41, -55, -20, -25, -32, -56, -26, -24, -39, -61, -24, -54, -51, -44};
    return PstItem{values, s};
}();
PstItem queen_pst = []() {
    Symmetry s = Symmetry::EW;
    std::vector<int> values{-38, -21, -25, 15, -23, 0, 24, 10, -17, 12, -16, -2, -18, 12, 1, -14, -7, 20, -1, 9, 5, 12, 26, 22, 4, -14, 5, 18, -4, 0, 8, 15};
    return PstItem{values, s};
}();
PstItem rook_pst = []() {
    Symmetry s = Symmetry::EW;
    std::vector<int> values{-7, 7, 19, 25, -23, 20, 17, 20, 10, 13, 19, 8, 12, 19, 19, 20, 15, 18, 18, 18, 15, 19, 19, 17, 25, 30, 30, 30, 20, 20, 17, 18};
    return PstItem{values, s};
}();

PstItem knight_pst = []() {
    Symmetry s = Symmetry::EW;
    std::vector<int> values{-38, -37, -36, -35, -29, -10, -19, 18, -47, -1, 19, 4,  -15, -3,  21,  27,
                            -15, 0,   17,  35,  -35, 18,  20,  12, -46, -7, -2, -1, -60, -26, -23, -1};
    return PstItem{values, s};
}();
PstItem bishop_pst = []() {
    Symmetry s = Symmetry::EW;
    std::vector<int> values{-36, 4, -13, -21, 7, 26, 15, 20, 2, 7, 23, 20, -2, 16, 18, 29, -1, -6, 14, 24, -4, 14, 0, 12, -21, 4, 17, -13, -2, 9, 9, 2};
    return PstItem{values, s};
}();

PstItem pawn_pst = []() {
    Symmetry s = Symmetry::EW;
    std::vector<int> values{-14, 5, 0, -30, -13, 2, -4, -7, -10, -4, -8, -1, 8, 18, 4, 7, 30, 31, 42, 51, 74, 75, 71, 74};
    return PstItem{values, s};
}();
PstItem king_pst_endgame = []() {
    Symmetry s = Symmetry::EW;
    std::vector<int> values = {-63, -45, -46, -43, -45, -15, 8,  15, -47, 6,  24, 40, -46, 6,   36,  44,
                               -28, 4,   26,  42,  -14, 6,   36, 22, -21, -2, 6,  10, -65, -25, -30, -9};
    return PstItem{values, s};
}();

extern const std::array<PstItem, 7> psq_parameters_base = {
    PstItem{std::vector<int>(), Symmetry::None}, king_pst, queen_pst, rook_pst, knight_pst, bishop_pst, pawn_pst

};

extern const std::array<std::optional<PstItem>, 7> psq_parameters_base_endgame = {
    std::nullopt, std::make_optional(king_pst_endgame), std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
} // namespace Eval::HCE