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
     * @return true : Successfully parsed FEN
     * @return false : Did not succesfully parse FEN. BoardState undefined
     */
    static bool read_fen(const std::string &FEN, BoardState& state);
};








#endif