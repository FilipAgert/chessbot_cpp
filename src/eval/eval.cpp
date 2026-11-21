#include "eval/eval.h"
namespace Eval {
bool forced_draw_ply(const Board &board) {
    if (board.get_ply_moves() >= 100)
        return true;
    else
        return false;
}
std::optional<int> moves_to_mate(int score) {
    // Negative score should remain negative.
    int dist = abs(abs(score) - MATE_SCORE);
    int move_to_mate = (dist + 1) / 2; // from ply to moves.
    if (move_to_mate < 100) {
        return std::make_optional(move_to_mate * (score / abs(score)));
    } else {
        return {};
    }
}
template <bool is_white> bool side_cannot_win(const Board &board) {
    int num_pieces = board.get_num_pieces<is_white>();
    if (num_pieces > 2)
        return false; // can win

    if (num_pieces == 1)
        return true; // with king only

    return (board.get_piece_bb<pieces::bishop, is_white>() | board.get_piece_bb<pieces::knight, is_white>()) > 0;
    // if our only piece is a knight or a bishop, we cannot mate.
}
bool insufficient_mating_material(const Board &board) {
    int black_num_pieces = board.get_num_pieces<false>();
    int white_num_pieces = board.get_num_pieces<true>();
    if (black_num_pieces > 2 || white_num_pieces > 2)
        return false;
    // K vs K is drawn
    // K + B vs K is drawn
    // K + N vs K is drawn
    // K + B vs K + B is drawn if both bishop are same color.
    if (black_num_pieces == white_num_pieces) {
        if (black_num_pieces == 1)
            return true;

        // 2 pieces each: check if both are same color bishops
        BB wbishop = board.get_piece_bb<pieces::bishop, true>();
        BB bbishop = board.get_piece_bb<pieces::bishop, false>();
        // if both are on white squares (or neither) we have a forced draw.
        BB both = wbishop | bbishop;
        if (wbishop == 0 | bbishop == 0)
            return false;
        return BitBoard::bitcount(both & masks::white_squares) % 2 == 0;
    } else {
        // one has 1 one has 2
        // drawn if the second piece is a bishop or knight.
        if (white_num_pieces == 1) {
            return (board.get_piece_bb<pieces::bishop, false>() | board.get_piece_bb<pieces::knight, false>()) > 0;
        }
        return (board.get_piece_bb<pieces::bishop, true>() | board.get_piece_bb<pieces::knight, true>()) > 0;
    }
}

template bool side_cannot_win<true>(const Board &board);
template bool side_cannot_win<false>(const Board &board);
} // namespace Eval