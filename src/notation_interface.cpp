#include <notation_interface.h>
#include <sstream>

bool NotationInterface::read_fen(const std::string FEN, BoardState& state){
    int row = 7;
    int col = 0;
    int num_pieces = 0;

    bool success = true;

    // ---------------------

    for (size_t i = 0; i < FEN.size(); ++i) {
        char ch = FEN[i];
        if (ch == '/') continue;
        if (ch == ' ') {
            // reached end of board part
            break;
        }

        if (std::isdigit(ch)) {
            int num = ch - '0';
            col += num;
        } else {
            Piece p = Piece::piece_from_char(ch);
            if (!Piece::is_valid_piece(p)) {
                success = false;
                break;
            }

            int idx = Board::idx(row, col);
            state.board.add_piece(idx,p);
            state.piece_locations[num_pieces++] = idx;
            col++;
        }

        if (col > 7) {
            col = 0;
            row--;
        }
        if (row < 0)
            break;
    }

    // ---------------------------------------
    // 2. Parse remaining FEN fields
    //     side castle enpassant halfmove fullmove
    // ---------------------------------------
    std::istringstream iss(FEN);
    std::string boardPart, turnPart, castlePart, epPart, halfPart, movePart;

    iss >> boardPart >> turnPart >> castlePart >> epPart >> halfPart >> movePart;

    // Side to move
    if (turnPart == "w") state.turn_color = Piece::white;      // white
    else if (turnPart == "b") state.turn_color = Piece::black; // black
    else success = false;

    // Castling rights
    state.castling = 0;
    if (castlePart != "-") {
        for (char c : castlePart) {
            switch (c) {
                case 'K': state.castling |= BoardState::cast_white_kingside; break;
                case 'Q': state.castling |= BoardState::cast_white_queenside; break;
                case 'k': state.castling |= BoardState::cast_black_kingside; break;
                case 'q': state.castling |= BoardState::cast_black_queenside; break;
                default: success = false;
            }
        }
    }

    // En passant
    if (epPart == "-") {
        state.en_passant = false;
    } else {
        state.en_passant = true;
        state.en_passant_square = Board::idx_from_string(epPart);
        if(state.en_passant_square > 63) success = false;
    }

    // Halfmove clock
    state.ply_moves = std::stoi(halfPart);

    // Fullmove number
    state.full_moves = std::stoi(movePart);

    // ----------------------------
    // 3. Store results in state
    // ----------------------------

    state.num_pieces = num_pieces;


    return success;
}