// Copyright Filip Agert
#include "integer_representation.h"
#include "notation_interface.h"
#include <cassert>
#include <eval.h>
#include <iomanip>
#include <piece.h>
#include <sstream>
#include <stdexcept>
#include <string>

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
    return "none";
}
// Piece square tables
int PieceSquareTable::num_params(Symmetry s, Piece_t p) const {
    if (s == Symmetry::None)
        return 64 - (p == pieces::pawn ? 16 : 0);
    if (s == Symmetry::EW || s == Symmetry::NS)
        return 32 - (p == pieces::pawn ? 8 : 0);
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
void south_north_mirror(std::array<int, 64> &arr) {
    // pastes south onto north, mirrored
    for (int i = 0; i < 32; i++) {
        arr[NotationInterface::idx_flip(i)] = arr[i];
    }
    for (int row = 0; row < 4; row++) {
        for (int col = 7; col >= 0; col--) {
            int nrow = 4 + row;
            int nidx = NotationInterface::idx(nrow, col);
            // idx to paste to.
            int srow = 3 - row;
            int sidx = NotationInterface::idx(srow, col);
            arr[nidx] = arr[sidx];
        }
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
    } else if (sym_ == Symmetry::NS) {  // 32 pieces.
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
    if (sym_ != Symmetry::EW && sym_ != Symmetry::EWNS)
        return 8;
    return 4;
}
size_t PieceSquareTable::height() const {
    if (sym_ != Symmetry::NS && sym_ != Symmetry::EWNS)
        return 8;
    return 4;
}
Symmetry PieceSquareTable::symmetry() const { return sym_; }

std::string PieceSquareTable::to_string() const {
    const size_t w = width(), h = height();

    size_t cw = 5;  // inner cell width, same for every cell
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
            const bool stored = static_cast<size_t>(f) < w && static_cast<size_t>(r) < (h - offset) && static_cast<size_t>(r) >= offset;
            os << centered(stored ? std::to_string(params[r * w + f]) : "") << "|";
        }
        os << " " << (r + 1) << "\n" << sep;
    }
    os << "    ";
    for (int f = 0; f < 8; ++f)
        os << centered(std::string(1, char('a' + f))) << " ";
    os << "\n";
    return os.str();
}

std::vector<int> EvalParameters::to_vector() const {
    std::vector<int> v;
#define X(name, def) v.push_back(name);
    EVAL_SCALAR_PARAMS(X)
#undef X
    for (size_t p = 1; p < num_pieces; ++p)
        v.push_back(piecevals[p]);
    for (size_t p = 1; p < num_pieces; ++p)
        v.push_back(movevals[p]);
    for (size_t p = 1; p < num_pieces; ++p)
        v.insert(v.end(), psts[p].params.begin(), psts[p].params.end());
    return v;
}

void EvalParameters::from_vector(const std::vector<int> &v) {
    if (v.size() != num_params())
        throw std::invalid_argument("EvalParameters::from_vector: wrong size");
    size_t i = 0;
#define X(name, def) name = v[i++];
    EVAL_SCALAR_PARAMS(X)
#undef X
    for (size_t p = 1; p < num_pieces; ++p)
        piecevals[p] = v[i++];
    for (size_t p = 1; p < num_pieces; ++p)
        movevals[p] = v[i++];
    for (size_t p = 1; p < num_pieces; ++p) {
        const size_t n = psts[p].size();
        psts[p].set(psts[p].symmetry(), p, std::vector<int>(v.begin() + i, v.begin() + i + n));
        i += n;
    }
}

size_t EvalParameters::num_params() const { return to_vector().size(); }
std::string EvalParameters::to_string() const {
    std::ostringstream os;
#define X(name, def) os << #name << " = " << name << "\n";
    EVAL_SCALAR_PARAMS(X)
#undef X
    auto dump = [&](const char *label, const std::array<int, num_pieces> &a) {
        os << label << " = {";
        for (size_t p = 1; p < num_pieces; ++p)
            os << a[p] << (p + 1 < num_pieces ? ", " : "");
        os << "}\n";
    };
    dump("piecevals[K,Q,R,N,B,P]", piecevals);
    dump("movevals[K,Q,R,N,B,P]", movevals);
    static const char *names[num_pieces] = {"", "King", "Queen", "Rook", "Knight", "Bishop", "Pawn"};
    for (size_t p = 1; p < num_pieces; ++p) {
        os << "\n" << names[p] << " PST\n";
        os << psts[p].to_string();
        os << "\n";
    }
    return os.str();
}

EvalState::EvalState(EvalParameters params_) : params(params_) {}
EvalState::EvalState() : params(EvalParameters()) {}

int EvalState::eval(Board &board) {
    int score = 0;
    if (forced_draw_ply(board))
        return 0;
    // Eval by piece scoring.
    score += eval_material(board);
    score += eval_mobility(board);
    score += eval_king_dist2centre(board);
    score += eval_pawn_structure(board);

    int color_fac = 1 - 2 * (board.get_turn_color() == pieces::black);
    return score * color_fac;
}

int EvalState::eval_mobility(Board &board) {
    constexpr bool omit_pawn = true;
    int king_mobility = board.get_piece_mobility<pieces::king, omit_pawn, true>() - board.get_piece_mobility<pieces::king, omit_pawn, false>();
    int bishop_mobility = board.get_piece_mobility<pieces::bishop, omit_pawn, true>() - board.get_piece_mobility<pieces::bishop, omit_pawn, false>();
    // int queen_mobility = board.get_piece_mobility<pieces::queen, omit_pawn, true>() -
    //                      board.get_piece_mobility<pieces::queen, omit_pawn, false>();
    int knight_mobility = board.get_piece_mobility<pieces::knight, omit_pawn, true>() - board.get_piece_mobility<pieces::knight, omit_pawn, false>();
    int mobility_eval =
        king_mobility * params.movevals[pieces::king] + bishop_mobility * params.movevals[pieces::bishop] + knight_mobility * params.movevals[pieces::knight];
    // queen_mobility * PieceValue::movevals[pieces::queen];
    return mobility_eval;
}
bool EvalState::forced_draw_ply(Board &board) {
    if (board.get_ply_moves() >= 100)
        return true;
    else
        return false;
}
int EvalState::eval_material(Board &board) {
    int score = 0;
    score += eval_single_piece<pieces::king>(board);
    score += eval_single_piece<pieces::queen>(board);
    score += eval_single_piece<pieces::rook>(board);
    score += eval_single_piece<pieces::bishop>(board);
    score += eval_single_piece<pieces::knight>(board);
    score += eval_single_piece<pieces::pawn>(board);
    return score;
}
template <Piece_t p> int EvalState::eval_single_piece(Board &board) {
    int pvalue = params.piecevals[p];
    BB wpieces = board.get_piece_bb<p, true>();
    BB bpieces = board.get_piece_bb<p, false>();
    const PieceSquareTable &pst = params.psts[p];
    int eval = 0, wpiece_cnt = 0, bpiece_cnt = 0;
    BitLoop(wpieces) {
        eval += pst.value<true>(BitBoard::lsb(wpieces));
        wpiece_cnt++;
    }
    BitLoop(bpieces) {
        eval -= pst.value<false>(BitBoard::lsb(bpieces));
        bpiece_cnt++;
    }
    eval += (wpiece_cnt - bpiece_cnt) * pvalue;
    if constexpr (p == pieces::bishop) {
        if (wpiece_cnt > 1)
            eval += params.bishop_double_bonus;
        if (bpiece_cnt > 1)
            eval -= params.bishop_double_bonus;
    }
    return eval;
}
int EvalState::eval_king_dist2centre(Board &board) {
    uint8_t white_king_sq = BitBoard::lsb(board.get_piece_bb<pieces::king, true>());
    uint8_t black_king_sq = BitBoard::lsb(board.get_piece_bb<pieces::king, false>());
    uint8_t king_dist = helpers::manhattan(white_king_sq, black_king_sq);
    float white_endgame = 2 * eval_game_phase(board.get_num_pieces<false>()) - 1;  // eval based on black pieces
    float black_endgame = 2 * eval_game_phase(board.get_num_pieces<true>()) - 1;   // eval based on white pieces

    float relval = king_dist * (white_endgame - black_endgame) * params.king_dist;
    return static_cast<int>(relval);
}
std::optional<int> EvalState::moves_to_mate(int score) {
    // Negative score should remain negative.
    int dist = abs(abs(score) - MATE_SCORE);
    int move_to_mate = (dist + 1) / 2;  // from ply to moves.
    if (move_to_mate < 100) {
        return std::make_optional(move_to_mate * (score / abs(score)));
    } else {
        return {};
    }
}
int EvalState::eval_pawn_structure(Board &board) {
    constexpr int maxforward = 4;
    constexpr BB AFILE = ~masks::left;
    constexpr BB HFILE = ~masks::right;

    BB wpawns = board.get_piece_bb<pieces::pawn, true>();
    BB bpawns = board.get_piece_bb<pieces::pawn, false>();

    BB w_N_fill = wpawns << 8;
    BB b_S_fill = bpawns >> 8;
    for (int i = 0; i < maxforward; i++) {
        w_N_fill |= w_N_fill << 8;
        b_S_fill |= b_S_fill >> 8;
    }

    BB w_NE_fill = (HFILE & w_N_fill) << 1;
    BB b_SE_fill = (HFILE & b_S_fill) << 1;

    BB w_NW_fill = (AFILE & w_N_fill) >> 1;
    BB b_SW_fill = (AFILE & b_S_fill) >> 1;

    BB w_full_mask = w_NE_fill | w_NW_fill | w_N_fill;
    BB b_full_mask = b_SE_fill | b_SW_fill | b_S_fill;

    int bpassed = BitBoard::bitcount(~w_full_mask & bpawns);  // Counts black pawns not in the white passed
                                                              // mask. these are all passed black pawns.
    int wpassed = BitBoard::bitcount(~b_full_mask & wpawns);  // Counts white pawns not in the black passed mask
    assert(bpassed <= 8);
    assert(wpassed <= 8);
    assert(bpassed >= 0);
    assert(wpassed >= 0);
    // END PASSED PAWNS
    // CALCULATE DOUBLED PAWNS
    int wdoubled = BitBoard::bitcount(w_N_fill & wpawns);
    int bdoubled = BitBoard::bitcount(b_S_fill & bpawns);

    // Calculate solo pawns (no friendly pawns on adjacent files)
    BB w_fill = w_N_fill;
    BB b_fill = b_S_fill;
    for (int i = 0; i < maxforward; i++) {
        w_fill |= w_fill >> 8;
        b_fill |= b_fill >> 8;
    }
    w_NE_fill = (HFILE & w_fill) << 1;
    b_SE_fill = (HFILE & b_fill) << 1;

    w_NW_fill = (AFILE & w_fill) >> 1;
    b_SW_fill = (AFILE & b_fill) >> 1;

    BB w_sides = w_NE_fill | w_NW_fill;
    BB b_sides = w_NE_fill | w_NW_fill;

    // Number of alone pawns (no pawns on adjacent files.)
    int nsolow = BitBoard::bitcount(wpawns) - BitBoard::bitcount(w_sides & wpawns);
    int nsolob = BitBoard::bitcount(bpawns) - BitBoard::bitcount(b_sides & bpawns);

    return (wpassed - bpassed) * params.passed_pawn_eval + (wdoubled - bdoubled) * params.doubled_pawn_punishment +
           (nsolow - nsolob) * params.solo_pawn_punishment;
}

/**
 * Collection of piece square tables.
 *
 */
using PstItem = std::pair<std::vector<int>, Symmetry>;
PstItem pawn_pst = []() {
    Symmetry s = Symmetry::EW;
    std::vector<int> values{5, 10, 10, -20, 5, -5, -10, 0, 0, 0, 0, 20, 5, 5, 10, 25, 10, 10, 20, 30, 50, 50, 50, 50};
    return PstItem{values, s};
}();
PstItem knight_pst = []() {
    Symmetry s = Symmetry::EWNS;
    std::vector<int> values{-50, -40, -30, -30, -40, -20, 0, 5, -30, 5, 10, 15, -30, 0, 15, 20};
    return PstItem{values, s};
}();
PstItem bishop_pst = []() {
    Symmetry s = Symmetry::EW;
    std::vector<int> values{-20, -10, -10, -10, -10, 5, 0, 0, -10, 10, 10, 10, -10, 0, 10, 10, -10, 5, 5, 10, -10, 0, 5, 10, -10, 0, 0, 0, -20, -10, -10, -10};
    return PstItem{values, s};
}();
PstItem rook_pst = []() {
    Symmetry s = Symmetry::EW;
    std::vector<int> values{0, 0, 0, 5, -5, 0, 0, 0, -5, 0, 0, 0, -5, 0, 0, 0, -5, 0, 0, 0, -5, 0, 0, 0, 5, 10, 10, 10, 0, 0, 0, 0};
    return PstItem{values, s};
}();
PstItem queen_pst = []() {
    Symmetry s = Symmetry::EWNS;
    std::vector<int> values{-20, -10, -10, -5, -10, 0, 5, 0, -10, 5, 5, 5, 0, 0, 5, 5};
    return PstItem{values, s};
}();
PstItem king_pst = []() {
    Symmetry s = Symmetry::EW;
    std::vector<int> values{20,  30,  10,  0,   20,  20,  0,   0,   -10, -20, -20, -20, -20, -30, -30, -40,
                            -30, -40, -40, -50, -30, -40, -40, -50, -30, -40, -40, -50, -30, -40, -40, -50};
    return PstItem{values, s};
}();

extern const std::array<PstItem, 7> psq_parameters_base = {
    PstItem{std::vector<int>(), Symmetry::None}, king_pst, queen_pst, rook_pst, knight_pst, bishop_pst, pawn_pst

};