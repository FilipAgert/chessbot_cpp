#include "eval/hce/hceval.h"
#include "eval.h"
#include "eval/hce/utils.h"
#include "integer_representation.h"
#include "parameters.h"
#include <algorithm>
#include <cmath>
#include <type_traits>
namespace Eval::HCE {
HCEval::HCEval(EvalParameters params_) : params(params_) {}

const HCE::EvalParameters &HCEval::get_params() const { return params; }

void HCEval::pop_move(Move move, const Board &board) {}
void HCEval::push_move(Move move, const Board &board) {}
template <typename Acc> void HCEval::acc_features(const Board &board, Acc &acc) {
    if constexpr (std::is_same_v<Acc, FeatureAccumulator>) {
        int phase = Utils::get_phase(board);
        acc.phase = phase;
    }
    int mat_diff = eval_material_diff(board, acc);
    eval_pst(board, acc);
    eval_mobility(board, acc);
    eval_king_safety(board, acc);
    eval_king_distance(board, mat_diff, acc);
    eval_pawn_structure(board, acc);
}
int HCEval::evaluate(const Board &board) {
    if (forced_draw_ply(board))
        return 0;
    // Eval by piece scoring.
    ScoreAccumulator acc{0, 0, 0, params};
    acc_features(board, acc);
    int phase = Utils::get_phase(board);
    int score = acc.shared + Utils::eval_phase_score(phase, acc.mg, acc.eg);

    if (side_cannot_win<true>(board)) {
        score = std::min(score, 0);
    }

    if (side_cannot_win<false>(board))
        score = std::max(score, 0);

    int color_fac = 1 - 2 * (board.get_turn_color() == pieces::black);
    return score * color_fac;
}
template <typename Acc> void HCEval::eval_mobility(const Board &board, Acc &acc) {
    constexpr bool omit_pawn = true;
    int king_mobility = board.get_piece_mobility<pieces::king, omit_pawn, true>() - board.get_piece_mobility<pieces::king, omit_pawn, false>();
    int bishop_mobility = board.get_piece_mobility<pieces::bishop, omit_pawn, true>() - board.get_piece_mobility<pieces::bishop, omit_pawn, false>();
    int queen_mobility = board.get_piece_mobility<pieces::queen, omit_pawn, true>() - board.get_piece_mobility<pieces::queen, omit_pawn, false>();
    int knight_mobility = board.get_piece_mobility<pieces::knight, omit_pawn, true>() - board.get_piece_mobility<pieces::knight, omit_pawn, false>();
    int rook_mobility = board.get_piece_mobility<pieces::rook, omit_pawn, true>() - board.get_piece_mobility<pieces::rook, omit_pawn, false>();
    int pawn_mobility = board.get_piece_mobility<pieces::pawn, false, true>() - board.get_piece_mobility<pieces::pawn, false, false>();
    acc.template add<true, Feature::Moveval, pieces::king>(king_mobility);
    acc.template add<true, Feature::Moveval, pieces::bishop>(bishop_mobility);
    acc.template add<true, Feature::Moveval, pieces::queen>(queen_mobility);
    acc.template add<true, Feature::Moveval, pieces::rook>(rook_mobility);
    acc.template add<true, Feature::Moveval, pieces::knight>(knight_mobility);
    acc.template add<true, Feature::Moveval, pieces::pawn>(pawn_mobility);
}

template <typename Acc, Piece_t p> void HCEval::eval_pst(const Board &board, Acc &acc) {
    if constexpr (p <= 6) {
        eval_single_pst<Acc, p>(board, acc);
        eval_pst<Acc, p + 1>(board, acc);
    }
}
template <typename Acc, Piece_t p> int HCEval::eval_material_diff(const Board &board, Acc &acc) {
    if constexpr (p <= 6) {
        int wpiece_cnt = board.get_num_pieces<true, p>();
        int bpiece_cnt = board.get_num_pieces<false, p>();
        int diff = (wpiece_cnt - bpiece_cnt);

        acc.template add<true, Feature::Pieceval, p>(diff);
        if constexpr (p == pieces::bishop) {
            int bishop_feature = 0;
            if (wpiece_cnt > 1) {
                bishop_feature++;
            }
            if (bpiece_cnt > 1) {
                bishop_feature--;
            }
            acc.template add<true, Feature::bishop_double_bonus>(bishop_feature);
        }
        return diff * piecevals_basic[p] + eval_material_diff<Acc, p + 1>(board, acc);
    } else {
        return 0;
    }
}
template <typename Acc, Piece_t p> void HCEval::eval_single_pst(const Board &board, Acc &acc) {
    BB wpieces = board.get_piece_bb<p, true>();
    BB bpieces = board.get_piece_bb<p, false>();
    BitLoop(wpieces) {
        int sq = BitBoard::lsb(wpieces);
        acc.template add<true, Feature::PST, p>(1, sq);
    }
    BitLoop(bpieces) {
        int sq = BitBoard::lsb(bpieces);
        acc.template add<false, Feature::PST, p>(1, sq);
    }
}
template <typename Acc> void HCEval::eval_king_distance(const Board &board, int mat_diff, Acc &acc) {
    uint8_t white_king_sq = BitBoard::lsb(board.get_piece_bb<pieces::king, true>());
    uint8_t black_king_sq = BitBoard::lsb(board.get_piece_bb<pieces::king, false>());
    uint8_t king_dist = Utils::manhattan(white_king_sq, black_king_sq);
    mat_diff = std::clamp(mat_diff, -400, 400);

    int to_scale = -mat_diff * king_dist;
    acc.template add<true, Feature::king_dist>(to_scale >> 9);
}
template <typename Acc> void HCEval::eval_king_safety(const Board &board, Acc &acc) {
    // king safety evaluation...
    // eval if we have friendly pieces near us.
    //    - Pawns in front of us.
    bool white_has_queen = board.get_piece_bb<pieces::queen, true>() > 0;
    bool black_has_queen = board.get_piece_bb<pieces::queen, false>() > 0;

    int pawn_proximity = 0;
    if (black_has_queen) {
        int wkingsq = BitBoard::lsb(board.get_piece_bb<pieces::king, true>());
        BB wking_front = masks::NS_wall_lookup[wkingsq + 64];
        pawn_proximity += BitBoard::bitcount(wking_front & board.get_piece_bb<pieces::pawn, true>());
    }
    if (white_has_queen) {
        int bkingsq = BitBoard::lsb(board.get_piece_bb<pieces::king, false>());
        BB bking_front = masks::NS_wall_lookup[bkingsq];
        pawn_proximity -= BitBoard::bitcount(bking_front & board.get_piece_bb<pieces::pawn, false>());
    }
    acc.template add<true, Feature::king_pawn_proximity_score>(pawn_proximity);

    // eval squares under attack near us
    BB white_attacks = board.get_atk_bb<true, false>();
    BB black_attacks = board.get_atk_bb<false, false>();
    uint8_t wkingsq = BitBoard::lsb(board.get_piece_bb<pieces::king, true>());
    uint8_t bkingsq = BitBoard::lsb(board.get_piece_bb<pieces::king, false>());
    int w_danger = BitBoard::bitcount(black_attacks & masks::three_x_three[wkingsq]);
    int b_danger = BitBoard::bitcount(white_attacks & masks::three_x_three[bkingsq]);
    acc.template add<true, Feature::king_attacked_square_score>(w_danger - b_danger);
}

template <typename Acc> void HCEval::eval_pawn_structure(const Board &board, Acc &acc) {
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

    BitLoop(wpassed) { acc.template add<true, Feature::PassedDistToPromo, pieces::pawn>(1, BitBoard::lsb(wpassed)); }
    BitLoop(bpassed) { acc.template add<false, Feature::PassedDistToPromo, pieces::pawn>(1, BitBoard::lsb(bpassed)); }
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
    acc.template add<true, Feature::doubled_pawn_punishment>(wdoubled - bdoubled);
    acc.template add<true, Feature::solo_pawn_punishment>(nsolow - nsolob);
}

template <bool is_white> bool HCEval::delta_futility_pruning_(int standing_pat, int alpha, int threshhold, const Board &board) {
    const int queen = params.shared.piecevals[pieces::queen];
    Piece_t most_valuable_piece =
        board.get_piece_bb<pieces::queen, !is_white>() > 0
            ? pieces::queen
            : (board.get_piece_bb<pieces::rook, !is_white>() > 0                                                         ? pieces::rook
               : (board.get_piece_bb<pieces::bishop, !is_white>() | board.get_piece_bb<pieces::knight, !is_white>()) > 0 ? pieces::bishop
                                                                                                                         : pieces::pawn);
    int big_delta = params.shared.piecevals[most_valuable_piece] + threshhold;
    if (board.has_pawn_promotion<is_white>()) {
        big_delta += queen - params.shared.piecevals[pieces::pawn];
    }
    return standing_pat + big_delta < alpha;
}
bool HCEval::delta_futility_pruning(int standing_pat, int alpha, int threshhold, const Board &board) {
    bool is_white = board.get_turn_color() == pieces::white;
    if (is_white)
        return delta_futility_pruning_<true>(standing_pat, alpha, threshhold, board);

    return delta_futility_pruning_<false>(standing_pat, alpha, threshhold, board);
}

template void HCEval::acc_features(const Board &board, ScoreAccumulator &acc);
template void HCEval::acc_features(const Board &board, FeatureAccumulator &acc);
} // namespace Eval::HCE