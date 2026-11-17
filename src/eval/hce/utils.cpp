#include "eval/hce/utils.h"
namespace Eval::HCE::Utils {
template <bool one_sided, bool is_white> int get_phase(const Board &board) {
    constexpr int pawn_phase = 0;
    constexpr int knight_phase = 1;
    constexpr int bishop_phase = 1;
    constexpr int rook_phase = 2;
    constexpr int queen_phase = 4;
    constexpr int total_phase = 16 * pawn_phase + 4 * knight_phase + 4 * bishop_phase + 4 * rook_phase + 2 * queen_phase;
    constexpr int max = one_sided ? 128 : 256;
    int phase = one_sided ? total_phase / 2 : total_phase;
    if constexpr ((one_sided && is_white) || !one_sided) {
        phase -= board.get_num_pieces<true, pieces::queen>() * queen_phase;
        phase -= board.get_num_pieces<true, pieces::rook>() * rook_phase;
        phase -= board.get_num_pieces<true, pieces::knight>() * knight_phase;
        phase -= board.get_num_pieces<true, pieces::bishop>() * bishop_phase;
        phase -= board.get_num_pieces<true, pieces::pawn>() * pawn_phase;
    }
    if constexpr ((one_sided && (!is_white)) || (!one_sided)) {
        phase -= board.get_num_pieces<false, pieces::queen>() * queen_phase;
        phase -= board.get_num_pieces<false, pieces::rook>() * rook_phase;
        phase -= board.get_num_pieces<false, pieces::knight>() * knight_phase;
        phase -= board.get_num_pieces<false, pieces::bishop>() * bishop_phase;
        phase -= board.get_num_pieces<false, pieces::pawn>() * pawn_phase;
    }
    phase = std::max(0, phase);
    return (phase * max + (total_phase / 2)) / total_phase;
}
template <bool one_sided> int eval_phase(int phase, int early_game, int late_game) {
    constexpr int max = one_sided ? 128 : 256;
    return (early_game * (max - phase) + late_game * phase) / max;
}

uint8_t manhattan(uint8_t sq1, uint8_t sq2) {
    uint8_t r1, c1, r2, c2;
    r1 = NotationInterface::row(sq1);
    r2 = NotationInterface::row(sq2);
    c1 = NotationInterface::col(sq1);
    c2 = NotationInterface::col(sq2);
    int rdist = static_cast<int>(r1) - static_cast<int>(r2);
    rdist = rdist < 0 ? -rdist : rdist;
    int cdist = static_cast<int>(c1) - static_cast<int>(c2);
    cdist = cdist < 0 ? -cdist : cdist;
    return rdist + cdist;
}

extern const std::array<uint8_t, 64> dist2centre = [] {
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

template <bool one_sided> int eval_phase_score(int phase, int early_game_score, int late_game_score){
    constexpr int max = one_sided ? 128 : 256;
    return (early_game_score * (max - phase) + late_game_score * phase) / max;
}

template int eval_phase_score<true>(int, int, int);
template int eval_phase_score<false>(int, int, int);
template int get_phase<true, true>(const Board& board);
template int get_phase<true, false>(const Board& board);
template int get_phase<false, true>(const Board& board);
template int get_phase<false,false>(const Board& board);
} // namespace Eval::HCE::Utils