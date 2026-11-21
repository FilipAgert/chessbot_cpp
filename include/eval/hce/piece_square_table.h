#pragma once
#include "core/piece.h"
#include <array>
#include <optional>
#include <vector>
namespace Eval::HCE {
enum class Symmetry : unsigned { None = 0, EW = 1, NS = 2, EWNS = 3, PIZZA };
std::string symmetry_name(Symmetry s);
Symmetry symmetry_from_name(std::string s);
extern const std::array<std::pair<std::vector<int>, Symmetry>, 7> psq_parameters_base;
extern const std::array<std::optional<std::pair<std::vector<int>, Symmetry>>, 7> psq_parameters_base_endgame;
struct PieceSquareTable {
    PieceSquareTable() { set(Symmetry::None, pieces::none, std::vector<int>(64, 0)); }
    /**
     * @brief Constructs a piece square table from a set of parameters and a symmetry
     *
     * @param s symmetry
     * @param p piece
     * @param params
     */
    PieceSquareTable(Symmetry s, Piece_t p, std::vector<int> params) { set(s, p, std::move(params)); }
    static int num_params(Symmetry s, Piece_t p);
    /**
     * @brief Set and recalculate.
     *
     * @param s
     * @param params
     */
    void set(Symmetry s, Piece_t p, std::vector<int> params);

    /**
     * @brief Gets the piece square table
     *
     * @tparam is_white
     * @return const std::array<int, 64>&
     */
    template <bool is_white> const std::array<int, 64> &get() {
        if constexpr (is_white)
            return white_;
        else
            return black_;
    }
    /**
     * @brief Gets value at square.
     *
     * @tparam is_white
     * @param sq
     * @return int
     */
    template <bool is_white> int value(int sq) const {
        if constexpr (is_white)
            return white_[sq];
        else
            return black_[sq];
    }

    std::vector<int> params;

    /**
     * @brief Number of parameters.
     *
     * @return size_t
     */
    size_t size() const;
    size_t width() const;
    size_t height() const;
    /**
     * @brief Gets symmetry.
     *
     * @return Symmetry
     */
    Symmetry symmetry() const;

    std::string to_string() const;

  private:
    Symmetry sym_;
    Piece_t piece_;
    std::array<int, 64> white_;
    std::array<int, 64> black_;

    static int num_cols(Symmetry s);
    /**
     * @brief Constructs the piece/square tables from parameters and symmetry.
     *
     */
    void rebuild();
};
} // namespace Eval::HCE