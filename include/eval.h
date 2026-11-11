// Copyrinht 2025 Filip Agert
#ifndef EVAL_H
#define EVAL_H
#include "piece.h"
#include <algorithm>
#include <array>
#include <board.h>
#include <vector>
namespace helpers {
/**
 * @brief Get manhattan distance between two squares
 *
 * @param[in] sq1 square1
 * @param[in] sq2 square2
 * @return manhattan distance between squares
 */
constexpr uint8_t manhattan(uint8_t sq1, uint8_t sq2) {
    uint8_t r1, c1, r2, c2;
    r1 = NotationInterface::row(sq1);
    r2 = NotationInterface::row(sq2);
    c1 = NotationInterface::col(sq1);
    c2 = NotationInterface::col(sq2);
    int rdist = static_cast<int>(r1) - static_cast<int>(r2);
    rdist = rdist < 0 ? -rdist : rdist;
    int cdist = static_cast<int>(c1) - static_cast<int>(c2);
    cdist = cdist < 0 ? -cdist : cdist;
    return rdist + cdist;
}
constexpr std::array<uint8_t, 64> dist2centre = [] {
    constexpr std::array<uint8_t, 4> centre_squares = {27, 28, 35, 36};
    std::array<uint8_t, 64> distances;
    for (int i = 0; i < 64; i++) {
        uint8_t dist = 64;
        for (uint8_t c : centre_squares) {
            dist = std::min(manhattan(c, i), dist);
        }
        distances[i] = dist;
    }
    return distances;
}();
}  // namespace helpers

// contains all parameters needed to evaluate board
namespace DefaultVals {
constexpr int king = 200000;
constexpr int pawn = 100;
constexpr int knight = 290;
constexpr int bishop = 300;
constexpr int rook = 500;
constexpr int queen = 900;

constexpr int inv_frac = 10;  // Fraction of extra value piece is worth extra from having more spaces to move to.
constexpr int pawn_moveval = 0;
constexpr int knight_moveval = knight / (8 * inv_frac);
constexpr int bishop_moveval = bishop / (14 * inv_frac);
constexpr int rook_moveval = rook / (14 * inv_frac);
constexpr int queen_moveval = 0;
constexpr int king_moveval = -6;

constexpr int bishop_double_bonus = 25;
constexpr int king_dist = 3;
constexpr int passed_pawn_eval = 30;
constexpr int doubled_pawn_punishment = -15;
constexpr int solo_pawn_punishment = -15;
}  // namespace DefaultVals
   //
enum class Symmetry : unsigned { None = 0, EW = 1, NS = 2, EWNS = 3 };
std::string symmetry_name(Symmetry s);
struct PieceSquareTable {
    PieceSquareTable() { set(Symmetry::None, pieces::none, std::vector<int>(64, 0)); }
    /**
     * @brief Constructs a piece square table from a set of parameters and a symmetry
     *
     * @param s symmetry
     * @param p piece
     * @param params
     */
    PieceSquareTable(Symmetry s, Piece_t p, std::vector<int> params) { set(s, p, std::move(params)); }
    int num_params(Symmetry s, Piece_t p) const;
    /**
     * @brief Set and recalculate.
     *
     * @param s
     * @param params
     */
    void set(Symmetry s, Piece_t p, std::vector<int> params);

    /**
     * @brief Gets the piece square table
     *
     * @tparam is_white
     * @return const std::array<int, 64>&
     */
    template <bool is_white> const std::array<int, 64> &get() {
        if constexpr (is_white)
            return white_;
        else
            return black_;
    }
    /**
     * @brief Gets value at square.
     *
     * @tparam is_white
     * @param sq
     * @return int
     */
    template <bool is_white> int value(int sq) const {
        if constexpr (is_white)
            return white_[sq];
        else
            return black_[sq];
    }

    std::vector<int> params;

    /**
     * @brief Number of parameters.
     *
     * @return size_t
     */
    size_t size() const;
    size_t width() const;
    size_t height() const;
    /**
     * @brief Gets symmetry.
     *
     * @return Symmetry
     */
    Symmetry symmetry() const;

    std::string to_string() const;

 private:
    Symmetry sym_;
    Piece_t piece_;
    std::array<int, 64> white_;
    std::array<int, 64> black_;

    static int num_cols(Symmetry s);
    /**
     * @brief Constructs the piece/square tables from parameters and symmetry.
     *
     */
    void rebuild();
};

extern const std::array<std::pair<std::vector<int>, Symmetry>, 7> psq_parameters_base;

#define EVAL_SCALAR_PARAMS(X)                                                                                                                                  \
    X(bishop_double_bonus, DefaultVals::bishop_double_bonus)                                                                                                   \
    X(king_dist, DefaultVals::king_dist)                                                                                                                       \
    X(doubled_pawn_punishment, DefaultVals::doubled_pawn_punishment)                                                                                           \
    X(solo_pawn_punishment, DefaultVals::solo_pawn_punishment)

struct EvalParameters {
#define X(name, def) int name = def;
    EVAL_SCALAR_PARAMS(X)
#undef X
    static const int num_pieces = 7;
    std::array<int, num_pieces> piecevals{
        0, DefaultVals::king, DefaultVals::queen, DefaultVals::rook, DefaultVals::knight, DefaultVals::bishop, DefaultVals::pawn};
    std::array<int, num_pieces> movevals{0,
                                         DefaultVals::king_moveval,
                                         DefaultVals::queen_moveval,
                                         DefaultVals::rook_moveval,
                                         DefaultVals::knight_moveval,
                                         DefaultVals::bishop_moveval,
                                         DefaultVals::pawn_moveval};
    std::array<PieceSquareTable, num_pieces> psts;
    std::array<int, 7> passed_dist_to_promotion = {0, 80, 60, 40, 30, 10, 10};

    void set_pst(Piece_t p, Symmetry s, std::vector<int> params) {
        assert(p > 0 && p < num_pieces);
        psts[p].set(s, p, std::move(params));
    }
    template <bool is_white> int pst(Piece_t p, int sq) const {
        assert(p > 0 && p < num_pieces);
        return psts.at(p).value<is_white>(sq);
    }

    std::vector<int> to_vector() const;
    void from_vector(const std::vector<int> &v);

    size_t num_params() const;
    std::string to_string() const;
    EvalParameters(std::array<std::pair<std::vector<int>, Symmetry>, num_pieces> psqs = psq_parameters_base) {
        for (Piece_t p = 1; p < num_pieces; p++) {
            set_pst(p, psqs.at(p).second, psqs.at(p).first);
        }
    }
};
class EvalState {
 public:
    int eval(Board &board);

    static constexpr int MATE_SCORE = 30000;
    static std::optional<int> moves_to_mate(int score);

    /**
     * @brief Evaluates true if state has forced draw from ply moves (gte 100 ply)
     *
     * @param[in] state staet
     * @return true if ply is grt or equal 100.
     */
    static bool forced_draw_ply(Board &board);
    template <bool is_white> static bool can_zugzwang(Board &board) {
        return board.occupancy<is_white>() == (board.get_piece_bb<pieces::king, is_white>() | board.get_piece_bb<pieces::pawn, is_white>());
    }
    EvalState(EvalParameters params_);
    EvalState();

    const EvalParameters params;

 private:
    int eval_material(Board &board);
    template <Piece_t p> int eval_single_piece(Board &board);

    int eval_mobility(Board &board);
    /**
     * @brief Evaluates king distance to centre score. At early game its negative, encouraging
     * safe play. Late game its positive, encouraging using the king.
     *
     * @param[in] state board state
     * @return evaluation of king distance to centre
     */
    int eval_king_dist2centre(Board &board);
    /**
     * @brief Gets game phase in [0, 1] by linear interpolation of the number of pieces
     *
     * @param[in] num_pieces Number of pieces and pawns
     * @return Game phase. 0 is early game and 1 is end game
     */
    constexpr static float eval_game_phase(const int num_pieces) { return 1. - (num_pieces - 1.) / 15.; }

    /**
     * @brief Score for pawn structure. Passed pawns, unprotected pawns, blocking pawns, etc.
     *
     * @param[in] board board to check.
     * @return score for pawn structure, normalised by white winning is positive.
     */
    int eval_pawn_structure(Board &board);
};

#endif
