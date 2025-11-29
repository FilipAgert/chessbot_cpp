#ifndef NOTATION_INTERFACE_H
#define NOTATION_INTERFACE_H

#include <constants.h>
#include <integer_representation.h>
#include <piece.h>

#include <string>

class NotationInterface {
 public:
    /**
     * @brief Gets index from square string e.g. "A5"
     */
    static uint8_t idx_from_string(std::string square);

    /**
     * @brief Gets algebraic notation from square.
     *
     * @param idx square idx [0-63]
     * @return std::string E.g. a5
     */
    static std::string string_from_idx(const uint8_t idx);

    static std::string starting_FEN() {
        std::string s = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        return s;
    }
    /**
     * @brief Fen standard string for printing out castling rights. '-' if no castling rights else
     * the sides.
     *
     * @param castle int containing the castling information
     * @return std::string
     */
    static std::string castling_rights(const uint8_t castle);
    /**
     * @brief Gets linear idx from row and col
     * @brief row (0-7)
     * @brief col (0-7)
     * @return linear idx [0-63
     */
    inline static uint8_t idx(uint8_t row, uint8_t col) { return col + row * 8; }

    /**
     * @brief Gets row, col from idx
     *
     * @param row Out: Row [0-7]
     * @param col Out: Col [0-7]
     * @param idx Square idx [0-63]
     */
    static void row_col(uint8_t &row, uint8_t &col, const uint8_t idx);
};

#endif
