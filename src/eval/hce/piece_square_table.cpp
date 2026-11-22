#include "eval/hce/piece_square_table.h"
#include "core/notation_interface.h"
#include <sstream>
#include <stdexcept>
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
    return "none";
}
Symmetry symmetry_from_name(std::string s) {
    if (s == "EW")
        return Symmetry::EW;
    return Symmetry::None;
}
// Piece square tables
int PieceSquareTable::num_params(Symmetry s, Piece_t p) {
    if (s == Symmetry::None)
        return 64 - (p == pieces::pawn ? 16 : 0);
    if (s == Symmetry::EW)
        return 32 - (p == pieces::pawn ? 8 : 0);
    throw std::runtime_error("Invalid symmetry");
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
    if (s == Symmetry::None)
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
    } else if (sym_ == Symmetry::EW) {
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
    if (sym_ != Symmetry::EW)
        return 8;
    return 4;
}
size_t PieceSquareTable::height() const { return 8; }
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
            const bool store = ((static_cast<size_t>(f) < w) && (static_cast<size_t>(r) < (h - offset)) && (static_cast<size_t>(r) >= offset));
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
std::vector<std::vector<int>> PieceSquareTable::get_parameter_groups(Symmetry s, Piece_t p) {

    std::vector<std::vector<int>> out(num_params(s, p));

    for (size_t i = 0; i < num_params(s, p); i++) {
        int per_square = s == Symmetry::EW ? 2 : 1;
        std::vector<int> to(per_square);

        if (s == Symmetry::EW) {
            // we want to map local indices to squares

            int row = i / 4 + (p == pieces::pawn ? 1 : 0);
            int col = i % 4;
            int secc = 7 - col;

            int sq = NotationInterface::idx(row, col);
            to[0] = sq;
            int sq2 = NotationInterface::idx(row, secc);
            to[1] = sq2;
        } else {
            int offset = p == pieces::pawn ? 8 : 0;
            int sq = i + offset;
            to[0] = sq;
        }
        out[i] = to;
    }
    return out;
}
} // namespace Eval::HCE