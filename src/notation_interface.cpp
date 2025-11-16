#include <notation_interface.h>


std::string NotationInterface::castling_rights(const uint8_t castle){
    std::string builder = "";
    if(castle & cast_white_kingside) builder.push_back(Piece(king|white).get_char());
    if(castle & cast_white_queenside) builder.push_back(Piece(queen|white).get_char());
    if(castle & cast_black_kingside) builder.push_back(Piece(king|black).get_char());
    if(castle & cast_black_queenside) builder.push_back(Piece(queen|black).get_char());
    if (builder.length() == 0) builder = "-";
    return builder;
}
uint8_t NotationInterface::idx_from_string(std::string square){
    if (square.length() != 2) { //Needs exactly two characters
        return err_val8;
    }
    char colchar = square[0];
    char rowchar = square[1];
    uint8_t basecol;
    uint8_t baserow;
    uint8_t colval;
    uint8_t rowval;

    if(colchar >= 'A' && colchar <= 'H') {
        basecol = 'A';
    } else if(colchar >= 'a' && colchar <= 'h') {
        basecol = 'a';
    } else {
        return err_val8;
    }
    colval =colchar - basecol;
    if(rowchar >= '1' && rowchar <= '8') {
        baserow = '1';
        rowval =rowchar - baserow;
    } else {
        return err_val8;
    }

    return idx(rowval, colval);
};

std::string NotationInterface::string_from_idx(const uint8_t idx){
    uint8_t row, col;
    row_col(row, col, idx);
    char r = 'a'+col;
    char c = '1'+row;
    std::string s = std::string() + r + c;
    return s;
}

void NotationInterface::row_col(uint8_t& row, uint8_t& col, const uint8_t idx){
    col = idx % 8;
    row = idx/8;
}