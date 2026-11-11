// Copyrinht 2025 Filip Agert
#ifndef EVAL_H
#define EVAL_H
#include "piece.h"
#include <algorithm>
#include <array>
#include <board.h>
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
constexpr int king_dist2centre_value = 3;
constexpr int passed_pawn_eval = 30;
constexpr int doubled_pawn_punishment = -15;
constexpr int solo_pawn_punishment = -15;
}  // namespace DefaultVals
struct EvalParameters {
 private:
 public:
    std::array<int, 7> piecevals = {0, DefaultVals::king, DefaultVals::queen, DefaultVals::rook, DefaultVals::knight, DefaultVals::bishop, DefaultVals::pawn};
    std::array<int, 7> movevals = {0,
                                   DefaultVals::king_moveval,
                                   DefaultVals::queen_moveval,
                                   DefaultVals::rook_moveval,
                                   DefaultVals::knight_moveval,
                                   DefaultVals::bishop_moveval,
                                   DefaultVals::pawn_moveval};
    int bishop_double_bonus = DefaultVals::bishop_double_bonus;
    int king_dist2centre_value = DefaultVals::king_dist2centre_value;
    int passed_pawn_eval = DefaultVals::passed_pawn_eval;
    int doubled_pawn_punishment = DefaultVals::doubled_pawn_punishment;
    int solo_pawn_punishment = DefaultVals::solo_pawn_punishment;

    EvalParameters() {}
    EvalParameters(std::array<int, 5> piecevals_, std::array<int, 6> movevals_, int bishop_double_bonus, int king_dist2centre_value, int passed_pawn_eval,
                   int doubled_pawn_punishment, int solo_pawn_punishment)
        : bishop_double_bonus(bishop_double_bonus), king_dist2centre_value(king_dist2centre_value), passed_pawn_eval(passed_pawn_eval),
          doubled_pawn_punishment(doubled_pawn_punishment), solo_pawn_punishment(solo_pawn_punishment) {
        piecevals[0] = 0;
        piecevals[1] = DefaultVals::king;
        for (size_t i = 0; i < piecevals_.size(); i++) {
            piecevals[i + 2] = piecevals_[i];
        }
        for (size_t i = 0; i < movevals_.size(); i++) {
            movevals[i + 1] = movevals_[i];
        }
    }
    std::string to_string() const;
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
