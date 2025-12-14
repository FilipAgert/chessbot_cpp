#ifndef NOTATION_INTERFACE_H
#define NOTATION_INTERFACE_H

#include <constants.h>
#include <integer_representation.h>
#include <piece.h>
#include <stdexcept>
#include <string>

class NotationInterface {
 public:
    /**
     * @brief Gets index from square string e.g. "A5"
     */
    inline constexpr static uint8_t idx_from_string(const std::string square) {
        if (square.length() != 2) {  // Needs exactly two characters
            return err_val8;
        }
        char colchar = square[0];
        char rowchar = square[1];
        uint8_t basecol;
        uint8_t baserow;
        uint8_t colval;
        uint8_t rowval;

        if (colchar >= 'A' && colchar <= 'H') {
            basecol = 'A';
        } else if (colchar >= 'a' && colchar <= 'h') {
            basecol = 'a';
        } else {
            throw std::invalid_argument("Col must be between a-h inclusive");
            return err_val8;
        }
        colval = colchar - basecol;
        if (rowchar >= '1' && rowchar <= '8') {
            baserow = '1';
            rowval = rowchar - baserow;
        } else {
            throw std::invalid_argument("Row must be between 1-8 inclusive");
            return err_val8;
        }

        return idx(rowval, colval);
    }

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
    /**
     * @brief Gets row from idx.
     *
     * @param idx Square idx [0-63]
     * @return row
     */
    inline constexpr static int row(const uint8_t idx) { return idx / 8; }
    inline constexpr static int col(const uint8_t idx) {
        return idx & 0b111;
    }  // Equivalent to mod 8
};

#endif
