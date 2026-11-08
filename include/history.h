#pragma once
#include <algorithm>
#include <move.h>
#include <array>
#include <piece.h>
#include <assert.h>
struct History{
    using HistoryArr = std::array<std::array<int, 64>, 6>;
    private:
    HistoryArr white_history;
    HistoryArr black_history;
    constexpr static std::array<int, 64> EMPTY = [](){
        std::array<int, 64> arr;
        for (size_t i = 0; i < 64; i++){
            arr[i] = 0;
        }
        return arr;
    }();
    constexpr static int MAX_HISTORY = 100'000;

    template<bool is_white>
    inline HistoryArr& arr(){
        if constexpr (is_white){
            return white_history;
        } else{
            return black_history;
        }
    }
    template<bool is_white>
    inline const HistoryArr& arr() const{
        if constexpr (is_white){
            return white_history;
        } else{
            return black_history;
        }
    }
    
    public:
    History();
    void reset_history();

    template <bool is_white>
    inline int get(Piece_t piece, uint8_t to) const {
        const HistoryArr& h = arr<is_white>();
        assert(piece > 0 && piece < 6);
        return h[piece-1][to];
    }
    
    template <bool is_white>
    inline void update(Piece_t piece, uint8_t to, int bonus){
        HistoryArr& h = arr<is_white>();
        int clamped_bonus = std::clamp(bonus, -MAX_HISTORY, MAX_HISTORY);
        assert(piece > 0 && piece < 6);
        h[piece-1][to] += clamped_bonus - h[piece-1][to]*abs(clamped_bonus) / MAX_HISTORY;
    }

    
    
};