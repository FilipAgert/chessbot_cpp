// Copyright Filip Agert
#include <cassert>
#include <eval.h>
#include <iomanip>
#include <piece.h>
#include <sstream>
#include <string>
EvalState::EvalState(EvalParameters params_) : params(params_) {}
EvalState::EvalState() : params(EvalParameters()) {}
std::string EvalParameters::to_string() const {
    std::ostringstream oss;

    // Helper lambda to format a single table row
    auto print_row = [&oss](const std::string &name, int val) {
        oss << " | " << std::left << std::setw(26) << name << " | " << std::right << std::setw(10) << val << " |\n";
    };

    // Helper lambda for table headers
    auto print_header = [&oss](const std::string &title) {
        oss << "\n=== " << title << " ===\n";
        oss << " +----------------------------+------------+\n";
        oss << " | Parameter                  |      Value |\n";
        oss << " +----------------------------+------------+\n";
    };

    auto print_footer = [&oss]() { oss << " +----------------------------+------------+\n"; };

    // --- Group 1: Piece Values (Skipping King at index 1) ---
    print_header("Piece Values");
    const char *piece_names[] = {"", "King", "Queen", "Rook", "Knight", "Bishop", "Pawn"};
    for (size_t i = 2; i < piecevals.size(); ++i) {
        print_row(piece_names[i], piecevals[i]);
    }
    print_footer();

    // --- Group 2: Move / Mobility Values ---
    print_header("Move / Mobility Values");
    const char *move_names[] = {"None", "King Move", "Queen Move", "Rook Move", "Knight Move", "Bishop Move", "Pawn Move"};
    for (size_t i = 1; i < movevals.size(); ++i) {
        print_row(move_names[i], movevals[i]);
    }
    print_footer();

    // --- Group 3: Positional Bonuses & Punishments ---
    print_header("Positional Bonuses & Punishments");
    print_row("Bishop Double Bonus", bishop_double_bonus);
    print_row("King Dist to Centre", king_dist2centre_value);
    print_row("Passed Pawn Eval", passed_pawn_eval);
    print_row("Doubled Pawn Punishment", doubled_pawn_punishment);
    print_row("Solo Pawn Punishment", solo_pawn_punishment);
    print_footer();

    return oss.str();
}
int EvalState::eval(Board &board) {
    int score = 0;
    if (forced_draw_ply(board))
        return 0;
    // Eval by piece scoring.
    score += eval_material(board);
    score += eval_mobility(board);
    score += eval_king_dist2centre(board);
    score += eval_pawn_structure(board);

    int color_fac = 1 - 2 * (board.get_turn_color() == pieces::black);
    return score * color_fac;
}

int EvalState::eval_mobility(Board &board) {
    constexpr bool omit_pawn = true;
    int king_mobility = board.get_piece_mobility<pieces::king, omit_pawn, true>() - board.get_piece_mobility<pieces::king, omit_pawn, false>();
    int bishop_mobility = board.get_piece_mobility<pieces::bishop, omit_pawn, true>() - board.get_piece_mobility<pieces::bishop, omit_pawn, false>();
    // int queen_mobility = board.get_piece_mobility<pieces::queen, omit_pawn, true>() -
    //                      board.get_piece_mobility<pieces::queen, omit_pawn, false>();
    int knight_mobility = board.get_piece_mobility<pieces::knight, omit_pawn, true>() - board.get_piece_mobility<pieces::knight, omit_pawn, false>();
    int mobility_eval =
        king_mobility * params.movevals[pieces::king] + bishop_mobility * params.movevals[pieces::bishop] + knight_mobility * params.movevals[pieces::knight];
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
    int pvalue = params.piecevals[p];
    int wpiece_cnt = board.get_piece_cnt<p, true>();
    int bpiece_cnt = board.get_piece_cnt<p, false>();
    int eval = (wpiece_cnt - bpiece_cnt) * pvalue;
    if (p == pieces::bishop) {
        if (wpiece_cnt > 1)
            eval += params.bishop_double_bonus;
        if (bpiece_cnt > 1)
            eval -= params.bishop_double_bonus;
    }
    return eval;
}
int EvalState::eval_king_dist2centre(Board &board) {
    uint8_t white_king_sq = BitBoard::lsb(board.get_piece_bb<pieces::king, true>());
    uint8_t black_king_sq = BitBoard::lsb(board.get_piece_bb<pieces::king, false>());
    uint8_t white_dist = helpers::dist2centre[white_king_sq];
    uint8_t black_dist = helpers::dist2centre[black_king_sq];
    uint8_t king_dist = helpers::manhattan(white_king_sq, black_king_sq);
    float white_endgame = 2 * eval_game_phase(board.get_num_pieces<false>()) - 1;  // eval based on black pieces
    float black_endgame = 2 * eval_game_phase(board.get_num_pieces<true>()) - 1;   // eval based on white pieces

    // Absolute king position value.
    float wval = -white_dist * params.king_dist2centre_value * white_endgame;
    float bval = black_dist * params.king_dist2centre_value * black_endgame;

    float relval = king_dist * (white_endgame - black_endgame) * params.king_dist2centre_value;
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

    int bpassed = BitBoard::bitcount(~w_full_mask & bpawns);  // Counts black pawns not in the white passed
                                                              // mask. these are all passed black pawns.
    int wpassed = BitBoard::bitcount(~b_full_mask & wpawns);  // Counts white pawns not in the black passed mask
    assert(bpassed <= 8);
    assert(wpassed <= 8);
    assert(bpassed >= 0);
    assert(wpassed >= 0);
    // END PASSED PAWNS
    // CALCULATE DOUBLED PAWNS
    int wdoubled = BitBoard::bitcount(w_N_fill & wpawns);
    int bdoubled = BitBoard::bitcount(b_S_fill & bpawns);

    // Calculate solo pawns (no friendly pawns on adjacent files)
    BB w_fill = w_N_fill;
    BB b_fill = b_S_fill;
    for (int i = 0; i < maxforward; i++) {
        w_fill |= w_fill >> 8;
        b_fill |= b_fill >> 8;
    }
    w_NE_fill = (HFILE & w_fill) << 1;
    b_SE_fill = (HFILE & b_fill) << 1;

    w_NW_fill = (AFILE & w_fill) >> 1;
    b_SW_fill = (AFILE & b_fill) >> 1;

    BB w_sides = w_NE_fill | w_NW_fill;
    BB b_sides = w_NE_fill | w_NW_fill;

    // Number of alone pawns (no pawns on adjacent files.)
    int nsolow = BitBoard::bitcount(wpawns) - BitBoard::bitcount(w_sides & wpawns);
    int nsolob = BitBoard::bitcount(bpawns) - BitBoard::bitcount(b_sides & bpawns);

    return (wpassed - bpassed) * params.passed_pawn_eval + (wdoubled - bdoubled) * params.doubled_pawn_punishment +
           (nsolow - nsolob) * params.solo_pawn_punishment;
}
