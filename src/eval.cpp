// Copyright Filip Agert
#include <algorithm>
#include <eval.h>
#include <vector>
int EvalState::eval(BoardState &state) {
    int score = 0;

    // Eval by piece scoring.
    std::vector<Piece> pieces = state.board.get_pieces();
    score += eval_material(pieces);
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
int EvalState::eval_material(std::vector<Piece> pieces) {
    int score = 0;
    for (Piece p : pieces) {
        int sign =
            2 * (p.get_color() == pieces::white) - 1;  // Evaluates to -1 if black or 1 if white.
        int val = PieceValue::piecevals[p.get_type()];
        // val += PieceValue::movevals[p.get_type()] *
        //        mvcnt;  // Evaluation from having a large amount of moves possible.
        score += sign * val;
    }
    return score;
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
