// Copyright Filip Agert
#include <algorithm>
#include <eval.h>
#include <vector>
int EvalState::eval(BoardState state) {
    int score = 0;

    // Eval by piece scoring.
    std::vector<std::pair<Piece, uint8_t>> piece_move_cnt =
        state.board.get_piece_num_moves(state.castling, 0);  // TODO: Handle en passant
    // PERFT: eval performance of get_piece_num_moves. expensive calculation.
    // std::vector<Piece> pieces = state.board.get_pieces();

    for (std::pair<Piece, uint8_t> pa : piece_move_cnt) {
        Piece p = pa.first;
        uint8_t mvcnt = pa.second;
        int sign =
            2 * (p.get_color() == pieces::white) - 1;  // Evaluates to -1 if black or 1 if white.
        int val = PieceValue::piecevals[p.get_type()];
        val += PieceValue::movevals[p.get_type()] *
               mvcnt;  // Evaluation from having a large amount of moves possible.
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
