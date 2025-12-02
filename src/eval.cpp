// Copyright Filip Agert
#include <eval.h>
#include <vector>
int EvalState::eval(BoardState state) {
    int score = 0;

    // Eval by piece scoring.
    std::vector<Piece> pieces = state.board.get_pieces();
    for (Piece p : pieces) {
        int sign =
            2 * (p.get_color() == pieces::white) - 1;  // Evaluates to -1 if black or 1 if white.
        int val = PieceValue::piecevals[p.get_type()];
        score += sign * val;
    }

    return score;
}
