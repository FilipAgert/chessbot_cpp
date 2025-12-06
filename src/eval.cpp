// Copyright Filip Agert
#include <algorithm>
#include <eval.h>
#include <piece.h>
#include <vector>
int EvalState::eval(BoardState &state) {
    int score = 0;
    if (forced_draw_ply(state))
        return 0;
    // Eval by piece scoring.
    score += eval_material(state);
    score += eval_mobility(state);
    score += eval_king_dist2centre(state);

    int color_fac = 1 - 2 * (state.turn_color == pieces::black);
    return score * color_fac;
}

int EvalState::eval_mobility(BoardState &state) {
    constexpr bool omit_pawn = true;
    int king_mobility = state.board.get_piece_mobility<pieces::king, omit_pawn, true>() -
                        state.board.get_piece_mobility<pieces::king, omit_pawn, false>();
    int bishop_mobility = state.board.get_piece_mobility<pieces::bishop, omit_pawn, true>() -
                          state.board.get_piece_mobility<pieces::bishop, omit_pawn, false>();
    // int queen_mobility = state.board.get_piece_mobility<pieces::queen, omit_pawn, true>() -
    //                      state.board.get_piece_mobility<pieces::queen, omit_pawn, false>();
    int knight_mobility = state.board.get_piece_mobility<pieces::knight, omit_pawn, true>() -
                          state.board.get_piece_mobility<pieces::knight, omit_pawn, false>();
    int mobility_eval = king_mobility * PieceValue::movevals[pieces::king] +
                        bishop_mobility * PieceValue::movevals[pieces::bishop] +
                        knight_mobility * PieceValue::movevals[pieces::knight];
    // queen_mobility * PieceValue::movevals[pieces::queen];
    return mobility_eval;
}
bool EvalState::forced_draw_ply(BoardState &state) {
    if (state.ply_moves >= 100)
        return true;
    else
        return false;
}
int EvalState::eval_material(BoardState &state) {
    int score = 0;
    score += eval_single_piece<pieces::king>(state);
    score += eval_single_piece<pieces::queen>(state);
    score += eval_single_piece<pieces::rook>(state);
    score += eval_single_piece<pieces::bishop>(state);
    score += eval_single_piece<pieces::knight>(state);
    score += eval_single_piece<pieces::pawn>(state);
    return score;
}
template <Piece_t p> int EvalState::eval_single_piece(BoardState &state) {
    int pvalue = PieceValue::piecevals[p];
    return (state.board.get_piece_cnt<p, true>() - state.board.get_piece_cnt<p, false>()) * pvalue;
}
void EvalState::partial_move_sort(std::array<Move, max_legal_moves> &moves,
                                  std::array<int, max_legal_moves> &scores, size_t num_moves,
                                  bool ascending) {
    std::vector<std::pair<Move, int>> zipped;

    for (size_t i = 0; i < num_moves; i++) {
        int s = scores[i];
        if (!ascending)
            s = s * (-1);
        zipped.push_back({moves[i], s});
    }

    std::sort(zipped.begin(), zipped.end(), [](auto a, auto b) {
        return a.second < b.second;
    });  // Lambda to specify sort by second (score).

    for (size_t i = 0; i < num_moves; i++) {
        moves[i] = zipped[i].first;
        int s = zipped[i].second;
        if (!ascending)
            s = s * (-1);

        scores[i] = s;
    }
}
int EvalState::eval_king_dist2centre(BoardState &state) {
    uint8_t white_king_sq = BitBoard::lsb(state.board.get_piece_bb<pieces::king, true>());
    uint8_t black_king_sq = BitBoard::lsb(state.board.get_piece_bb<pieces::king, false>());
    uint8_t white_dist = helpers::dist2centre[white_king_sq];
    uint8_t black_dist = helpers::dist2centre[black_king_sq];
    int value = white_dist * PieceValue::king_dist2centre_value -
                black_dist * PieceValue::king_dist2centre_value;
    return (1. * value * (2 * EvalState::eval_game_phase(state.get_num_pieces()) - 1.0));
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
