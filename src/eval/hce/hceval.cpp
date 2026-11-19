#include "eval/hce/hceval.h"
#include "eval/hce/utils.h"
#include <algorithm>
namespace Eval::HCE {
HCEval::HCEval(EvalParameters params_) : params(params_) {}
HCEval::HCEval() : params(EvalParameters()) {}

const HCE::EvalParameters &HCEval::get_params() const { return params; }

void HCEval::pop_move(Move move, const Board &board) {}
void HCEval::push_move(Move move, const Board &board) {}

int HCEval::evaluate(const Board &board) {
    int score = 0;
    if (forced_draw_ply(board))
        return 0;
    // Eval by piece scoring.
    int phase = Utils::get_phase(board);
    int mat_diff = eval_material_diff(board);
    score = mat_diff;
    score += eval_pst(board, phase);
    score += eval_mobility(board);
    score += eval_king_safety(board, phase);
    score += eval_king_distance(board, mat_diff, phase);
    score += eval_pawn_structure(board);

    int color_fac = 1 - 2 * (board.get_turn_color() == pieces::black);
    return score * color_fac;
}
int HCEval::eval_mobility(const Board &board) {
    constexpr bool omit_pawn = true;
    int king_mobility = board.get_piece_mobility<pieces::king, omit_pawn, true>() - board.get_piece_mobility<pieces::king, omit_pawn, false>();
    int bishop_mobility = board.get_piece_mobility<pieces::bishop, omit_pawn, true>() - board.get_piece_mobility<pieces::bishop, omit_pawn, false>();
    int queen_mobility = board.get_piece_mobility<pieces::queen, omit_pawn, true>() - board.get_piece_mobility<pieces::queen, omit_pawn, false>();
    int knight_mobility = board.get_piece_mobility<pieces::knight, omit_pawn, true>() - board.get_piece_mobility<pieces::knight, omit_pawn, false>();
    int mobility_eval = king_mobility * params.movevals[pieces::king] + bishop_mobility * params.movevals[pieces::bishop] +
                        knight_mobility * params.movevals[pieces::knight] + queen_mobility * params.movevals[pieces::queen];
    return mobility_eval;
}

template <Piece_t p> int HCEval::eval_pst(const Board &board, int phase) {
    if constexpr (p <= 6) {
        int early_game = eval_single_pst<p>(board, params.psts[p]);
        int pst = early_game;
        if (params.psts_endgame[p]) {
            int late_game = eval_single_pst<p>(board, params.psts_endgame[p].value());
            pst = Utils::eval_phase_score<false>(phase, early_game, late_game);
        }

        return pst + eval_pst<p + 1>(board, phase);
    } else {
        return 0;
    }
}
template <Piece_t p> int HCEval::eval_material_diff(const Board &board) const {
    if constexpr (p <= 6) {
        int wpiece_cnt = board.get_num_pieces<true, p>();
        int bpiece_cnt = board.get_num_pieces<false, p>();
        int eval = (wpiece_cnt - bpiece_cnt) * params.piecevals[p];
        if constexpr (p == pieces::bishop) {
            if (wpiece_cnt > 1)
                eval += params.bishop_double_bonus;
            if (bpiece_cnt > 1)
                eval -= params.bishop_double_bonus;
        }
        return eval + eval_material_diff<p + 1>(board);
    } else {
        return 0;
    }
}
template <Piece_t p> int HCEval::eval_single_pst(const Board &board, const PieceSquareTable &pst) {
    BB wpieces = board.get_piece_bb<p, true>();
    BB bpieces = board.get_piece_bb<p, false>();
    int eval = 0;
    BitLoop(wpieces) { eval += pst.value<true>(BitBoard::lsb(wpieces)); }
    BitLoop(bpieces) { eval -= pst.value<false>(BitBoard::lsb(bpieces)); }
    return eval;
}
int HCEval::eval_king_distance(const Board &board, int mat_diff, int phase) {
    uint8_t white_king_sq = BitBoard::lsb(board.get_piece_bb<pieces::king, true>());
    uint8_t black_king_sq = BitBoard::lsb(board.get_piece_bb<pieces::king, false>());
    uint8_t king_dist = Utils::manhattan(white_king_sq, black_king_sq);
    mat_diff = std::clamp(mat_diff, -400, 400);

    int score = -(params.king_dist * king_dist * mat_diff);
    return Utils::eval_phase_score<false>(phase, 0, score) >>
           9; // divide by 512. Pure late game max advantage is 8 centipawns per square. Early game is closer to 1-3 cp per square.
}
int HCEval::eval_king_safety(const Board &board, int phase) const {
    // king safety evaluation...
    // eval if we have friendly pieces near us.
    //    - Pawns in front of us.
    bool white_has_queen = board.get_piece_bb<pieces::queen, true>() > 0;
    bool black_has_queen = board.get_piece_bb<pieces::queen, false>() > 0;

    int eval = 0;
    if (black_has_queen) {
        int wkingsq = BitBoard::lsb(board.get_piece_bb<pieces::king, true>());
        BB wking_front = masks::NS_wall_lookup[wkingsq + 64];
        eval += BitBoard::bitcount(wking_front & board.get_piece_bb<pieces::pawn, true>());
    }
    if (white_has_queen) {
        int bkingsq = BitBoard::lsb(board.get_piece_bb<pieces::king, false>());
        BB bking_front = masks::NS_wall_lookup[bkingsq];
        eval -= BitBoard::bitcount(bking_front & board.get_piece_bb<pieces::pawn, false>());
    }
    eval = Utils::eval_phase_score<false>(phase, params.king_pawn_proximity_score * eval, 0);

    // // eval squares under attack near us
    // BB white_attacks = board.get_atk_bb<true, false>();
    // BB black_attacks = board.get_atk_bb<false, false>();
    // int w_danger = BitBoard::bitcount(black_attacks & masks::three_x_three[wkingsq]);
    // int b_danger = BitBoard::bitcount(white_attacks & masks::three_x_three[bkingsq]);
    // eval += (w_danger - b_danger) * params.king_attacked_square_score;
    return eval;
}

int HCEval::eval_pawn_structure(const Board &board) {
    int eval = 0;
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

    BB bpassed = ~w_full_mask & bpawns; // Counts black pawns not in the white passed
                                        // mask. these are all passed black pawns.
    BB wpassed = ~b_full_mask & wpawns; // Counts white pawns not in the black passed mask

    BitLoop(wpassed) {
        uint8_t row = NotationInterface::row(BitBoard::lsb(wpassed));
        uint8_t dist_to_promote = 7 - row;
        eval += params.passed_dist_to_promotion[dist_to_promote];
    }
    BitLoop(bpassed) {
        uint8_t row = NotationInterface::row(BitBoard::lsb(bpassed));
        eval -= params.passed_dist_to_promotion[row];
    }
    // END PASSED PAWNS
    // CALCULATE DOUBLED PAWNS
    int wdoubled = BitBoard::bitcount(w_N_fill & wpawns);
    int bdoubled = BitBoard::bitcount(b_S_fill & bpawns);

    // Calculate solo pawns (no friendly pawns on adjacent files)
    BB w_fill = w_N_fill;
    BB b_fill = b_S_fill;
    for (int i = 0; i < maxforward; i++) {
        w_fill |= w_fill >> 8;
        b_fill |= b_fill << 8;
    }
    BB w_E_fill = (HFILE & w_fill) << 1;
    BB b_E_fill = (HFILE & b_fill) << 1;

    BB w_W_fill = (AFILE & w_fill) >> 1;
    BB b_W_fill = (AFILE & b_fill) >> 1;

    BB w_sides = w_E_fill | w_W_fill;
    BB b_sides = b_E_fill | b_W_fill;

    // Number of alone pawns (no pawns on adjacent files.)
    int nsolow = BitBoard::bitcount(wpawns) - BitBoard::bitcount(w_sides & wpawns);
    int nsolob = BitBoard::bitcount(bpawns) - BitBoard::bitcount(b_sides & bpawns);

    return eval + (wdoubled - bdoubled) * params.doubled_pawn_punishment + (nsolow - nsolob) * params.solo_pawn_punishment;
}

template <bool is_white> bool HCEval::delta_futility_pruning_(int standing_pat, int alpha, int threshhold, const Board &board) {
    const int queen = params.piecevals[pieces::queen];
    Piece_t most_valuable_piece =
        board.get_piece_bb<pieces::queen, !is_white>() > 0
            ? pieces::queen
            : (board.get_piece_bb<pieces::rook, !is_white>() > 0                                                         ? pieces::rook
               : (board.get_piece_bb<pieces::bishop, !is_white>() | board.get_piece_bb<pieces::knight, !is_white>()) > 0 ? pieces::bishop
                                                                                                                         : pieces::pawn);
    int big_delta = params.piecevals[most_valuable_piece] + threshhold;
    if (board.has_pawn_promotion<is_white>()) {
        big_delta += queen - params.piecevals[pieces::pawn];
    }
    return standing_pat + big_delta < alpha;
}
bool HCEval::delta_futility_pruning(int standing_pat, int alpha, int threshhold, const Board &board) {
    bool is_white = board.get_turn_color() == pieces::white;
    if (is_white)
        return delta_futility_pruning_<true>(standing_pat, alpha, threshhold, board);

    return delta_futility_pruning_<false>(standing_pat, alpha, threshhold, board);
}

} // namespace Eval::HCE