#ifndef NOTATION_INTERFACE_H
#define NOTATION_INTERFACE_H

#include <iostream>
#include <string>
#include <board_state.h>

class NotationInterface{
public:

    /**
     * @brief Takes a FEN string and sets it into the board state.
     * 
     * @param FEN String containing FEN
     * @param state out: State of the FEN string.
     * @return true : Successfully parsed FEN.
     * @return false : Did not succesfully parse FEN. BoardState undefined
     */
    static bool read_fen(std::string FEN, BoardState& state);


    static std::string starting_FEN() {std::string s = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"; return s;} 
    /**
     * @brief Fen standard string for printing out castling rights. '-' if no castling rights else the sides.
     * 
     * @param castle int containing the castling information
     * @return std::string 
     */
    static std::string castling_rights(const uint8_t castle);
};


#endif