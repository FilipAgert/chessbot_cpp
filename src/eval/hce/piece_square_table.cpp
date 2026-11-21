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
Symmetry symmetry_from_name(std::string s) {
    if (s == "EW")
        return Symmetry::EW;
    if (s == "EWNS")
        return Symmetry::EWNS;
    if (s == "NS")
        return Symmetry::NS;
    if (s == "PIZZA")
        return Symmetry::PIZZA;
    return Symmetry::None;
}
// Piece square tables
int PieceSquareTable::num_params(Symmetry s, Piece_t p) {
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
} // namespace Eval::HCE