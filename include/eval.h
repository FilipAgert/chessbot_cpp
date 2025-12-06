// Copyright 2025 Filip Agert
#ifndef EVAL_H
#define EVAL_H
#include "board_state.h"
#include <algorithm>
#include <array>
/**
 * @brief Get manhattan distance between two squares
 *
 * @param[in] sq1 square1
 * @param[in] sq2 square2
 * @return manhattan distance between squares
 */
namespace helpers {
constexpr uint8_t manhattan(uint8_t sq1, uint8_t sq2) {
    uint8_t r1, c1, r2, c2;
    r1 = NotationInterface::row(sq1);
    r2 = NotationInterface::row(sq2);
    c1 = NotationInterface::col(sq1);
    c2 = NotationInterface::col(sq2);
    return std::abs(static_cast<int>(r1) - static_cast<int>(r2)) +
           std::abs(static_cast<int>(c1) - static_cast<int>(c2));
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
class EvalState {
 public:
    static int eval(BoardState &state);
    /**
     * @brief Sorts the moves in array by the scores in scores
     *
     * @param[[TODO:direction]] moves [TODO:description]
     * @param[[TODO:direction]] scores [TODO:description]
     * @param[[TODO:direction]] num_moves [TODO:description]
     * @param[in] ascending: bool to sort in ascending order. If false sorts by descending order.
     * True if minimiser, false if maximiser.
     */
    static void partial_move_sort(std::array<Move, max_legal_moves> &moves,
                                  std::array<int, max_legal_moves> &scores, size_t num_moves,
                                  bool ascending);

    static constexpr int MATE_SCORE = 30000;
    static std::optional<int> moves_to_mate(int score);

    /**
     * @brief Evaluates true if state has forced draw from ply moves (gte 100 ply)
     *
     * @param[in] state staet
     * @return true if ply is grt or equal 100.
     */
    static bool forced_draw_ply(BoardState &state);

 private:
    static int eval_material(std::vector<Piece> pieces);
    static int eval_mobility(BoardState &state);
    /**
     * @brief Evaluates king distance to centre score. At early game its negative, encouraging
     * safe play. Late game its positive, encouraging using the king.
     *
     * @param[in] state board state
     * @return evaluation of king distance to centre
     */
    static int eval_king_dist2centre(BoardState &state);
    /**
     * @brief Gets game phase in [0, 1] by linear interpolation of the number of pieces
     *
     * @param[in] num_pieces Number of pieces and pawns
     * @return Game phase. 0 is early game and 1 is end game
     */
    constexpr static float eval_game_phase(const int num_pieces) { return (num_pieces - 2.) / 30.; }
};

struct PieceValue {
    static constexpr int king = 200000;
    static constexpr int pawn = 100;
    static constexpr int knight = 290;
    static constexpr int bishop = 300;
    static constexpr int rook = 500;
    static constexpr int queen = 900;
    static constexpr std::array<int, 7> piecevals = {0, king, queen, rook, knight, bishop, pawn};

    static constexpr int inv_frac =
        10;  // Fraction of extra value piece is worth extra from having more spaces to move to.
    // Formula is : piece_val * num_moves / max_possible_moves * frac
    static constexpr int pawn_moveval = 0;
    static constexpr int knight_moveval = knight / (8 * inv_frac);
    static constexpr int bishop_moveval = bishop / (14 * inv_frac);
    static constexpr int rook_moveval = rook / (14 * inv_frac);
    static constexpr int queen_moveval = 0;
    static constexpr int king_moveval = -6;
    static constexpr std::array<int, 7> movevals = {
        0, king_moveval, queen_moveval, rook_moveval, knight_moveval, bishop_moveval, pawn_moveval};

    static constexpr int king_dist2centre_value = 20;  // 20 per distance.
};

#endif
