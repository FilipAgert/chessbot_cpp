#pragma once
#include <algorithm>
#include <array>
#include <assert.h>
#include <cstdint>
#include <limits>
#include <move.h>
#include <piece.h>
struct History {
    using HistoryArr = std::array<std::array<int16_t, 64>, 6>;

 private:
    HistoryArr white_history;
    HistoryArr black_history;
    constexpr static std::array<int16_t, 64> EMPTY = []() {
        std::array<int16_t, 64> arr;
        for (size_t i = 0; i < 64; i++) {
            arr[i] = 0;
        }
        return arr;
    }();
    constexpr static int MAX_HISTORY = 16384;
    static_assert(MAX_HISTORY < std::numeric_limits<int16_t>::max(), "This is used in moveordering where we store morescore as int16_t.");

    template <bool is_white> inline HistoryArr &arr() {
        if constexpr (is_white) {
            return white_history;
        } else {
            return black_history;
        }
    }
    template <bool is_white> inline const HistoryArr &arr() const {
        if constexpr (is_white) {
            return white_history;
        } else {
            return black_history;
        }
    }

 public:
    History();
    void reset_history();

    template <bool is_white> inline int16_t get(Piece_t piece, uint8_t to) const {
        const HistoryArr &h = arr<is_white>();
        assert(piece > 0 && piece <= 6);
        return h[piece - 1][to];
    }

    template <bool is_white> inline void update(Piece_t piece, uint8_t to, int bonus) {
        HistoryArr &h = arr<is_white>();
        int16_t clamped_bonus = std::clamp<int16_t>(bonus, -MAX_HISTORY, MAX_HISTORY);
        assert(piece > 0 && piece <= 6);
        h[piece - 1][to] += clamped_bonus - h[piece - 1][to] * abs(clamped_bonus) / MAX_HISTORY;
    }
};