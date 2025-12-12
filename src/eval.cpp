// Copyright Filip Agert
#include <algorithm>
#include <eval.h>
#include <piece.h>
#include <vector>
int EvalState::eval(Board &board) {
    int score = 0;
    if (forced_draw_ply(board))
        return 0;
    // Eval by piece scoring.
    score += eval_material(board);
    score += eval_mobility(board);
    score += eval_king_dist2centre(board);

    int color_fac = 1 - 2 * (board.get_turn_color() == pieces::black);
    return score * color_fac;
}

int EvalState::eval_mobility(Board &board) {
    constexpr bool omit_pawn = true;
    int king_mobility = board.get_piece_mobility<pieces::king, omit_pawn, true>() -
                        board.get_piece_mobility<pieces::king, omit_pawn, false>();
    int bishop_mobility = board.get_piece_mobility<pieces::bishop, omit_pawn, true>() -
                          board.get_piece_mobility<pieces::bishop, omit_pawn, false>();
    // int queen_mobility = board.get_piece_mobility<pieces::queen, omit_pawn, true>() -
    //                      board.get_piece_mobility<pieces::queen, omit_pawn, false>();
    int knight_mobility = board.get_piece_mobility<pieces::knight, omit_pawn, true>() -
                          board.get_piece_mobility<pieces::knight, omit_pawn, false>();
    int mobility_eval = king_mobility * PieceValue::movevals[pieces::king] +
                        bishop_mobility * PieceValue::movevals[pieces::bishop] +
                        knight_mobility * PieceValue::movevals[pieces::knight];
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
    int pvalue = PieceValue::piecevals[p];
    int wpiece_cnt = board.get_piece_cnt<p, true>();
    int bpiece_cnt = board.get_piece_cnt<p, false>();
    int eval = (wpiece_cnt - bpiece_cnt) * pvalue;
    if (p == pieces::bishop) {
        if (wpiece_cnt > 1)
            eval += PieceValue::bishop_double_bonus;
        if (bpiece_cnt > 1)
            eval -= PieceValue::bishop_double_bonus;
    }
    return eval;
}
int EvalState::eval_king_dist2centre(Board &board) {
    uint8_t white_king_sq = BitBoard::lsb(board.get_piece_bb<pieces::king, true>());
    uint8_t black_king_sq = BitBoard::lsb(board.get_piece_bb<pieces::king, false>());
    uint8_t white_dist = helpers::dist2centre[white_king_sq];
    uint8_t black_dist = helpers::dist2centre[black_king_sq];
    uint8_t king_dist = helpers::manhattan(white_king_sq, black_king_sq);
    float white_endgame =
        2 * eval_game_phase(board.get_num_pieces<false>()) - 1;  // eval based on black pieces
    float black_endgame =
        2 * eval_game_phase(board.get_num_pieces<true>()) - 1;  // eval based on white pieces

    // Absolute king position value.
    float wval = -white_dist * PieceValue::king_dist2centre_value * white_endgame;
    float bval = black_dist * PieceValue::king_dist2centre_value * black_endgame;

    float relval = king_dist * (white_endgame - black_endgame) * PieceValue::king_dist2centre_value;
    return static_cast<int>(wval + bval + relval);
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

    int passed = 0;
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

    int bpassed =
        BitBoard::bitcount(~w_full_mask & bpawns);  // Counts black pawns not in the white passed
                                                    // mask. these are all passed black pawns.
    int wpassed = BitBoard::bitcount(~b_full_mask &
                                     wpawns);  // Counts white pawns not in the black passed mask
    // END PASSED PAWNS
    // CALCULATE DOUBLED PAWNS
    int wdoubled = w_N_fill & wpawns;
    int bdoubled = b_S_fill & bpawns;

    return (wpassed - bpassed) * PieceValue::passed_pawn_eval +
           (wdoubled - bdoubled) * PieceValue::doubled_pawn_punishment;
}
