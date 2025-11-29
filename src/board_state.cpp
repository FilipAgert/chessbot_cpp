// Copyright 2025 Filip Agert
#include <board_state.h>
#include <iostream>
#include <string>

using namespace pieces;
void BoardState::do_move(Move &move) {
    move.castling_rights = this->castling;
    move.en_passant_square = this->en_passant ? this->en_passant_square : err_val8;
    move.check = check;
    ply_moves += 1;
    if (this->turn_color == black)
        full_moves += 1;
    change_turn();  // Changes turn color from white <-> black.
    Piece moved = board.get_piece_at(move.start_square);

    // Exception: en_passant.
    if (en_passant && moved.get_type() == pawn && move.end_square == en_passant_square) {
        move.captured_piece = Piece(pawn | turn_color);  // Can only capture pawns in en_passant.
    } else {
        move.captured_piece = this->board.get_piece_at(move.end_square);
    }

    if (move.captured_piece.get_value()) {
        if (en_passant && moved.get_type() == pawn && move.end_square == en_passant_square) {
            uint8_t captured_pawn_loc =
                (turn_color == black) ? move.end_square - 8 : move.end_square + 8;
            // Depends on turn color. If black, we captured a black piece. This means that the
            // location of the pawn is target_square-8,else +8
            board.capture_piece_ep(move.start_square, move.end_square, captured_pawn_loc);
        } else {
            board.capture_piece(move.start_square, move.end_square);
        }
    } else {
        board.move_piece(move.start_square, move.end_square);
    }
    if (move.promotion.get_value()) {
        board.promote_piece(move.end_square, move.promotion);
        if (move.promotion.get_color() == none) {
            std::cerr << "Invalid argument: Promoted piece must have a color. Piece value: "
                      << move.promotion.get_value() << "\n";
            std::abort();
        }
    }
    // Set the en_passant flags
    if (moved.get_type() == pawn && abs(move.end_square - move.start_square) == 16) {
        en_passant = true;
        en_passant_square = (move.start_square + move.end_square) / 2;
    } else {
        en_passant = false;
        en_passant_square = err_val8;
    }
}

void BoardState::undo_move(const Move move) {
    castling = move.castling_rights;
    en_passant_square = move.en_passant_square;
    en_passant = en_passant_square < 64;
    check = move.check;
    board.move_piece(move.end_square, move.start_square);
    if (move.promotion.get_value()) {
        board.promote_piece(move.start_square,
                            Piece(move.promotion.get_color() | pawn));  // Replace with pawn.
    }

    if (move.captured_piece.get_value()) {
        if (en_passant && board.get_piece_at(move.start_square).get_type() == pawn &&
            move.end_square == en_passant_square) {
            uint8_t captured_pawn_loc =
                (turn_color == black) ? move.end_square - 8 : move.end_square + 8;
            board.add_piece(captured_pawn_loc, move.captured_piece);
        } else {
            board.add_piece(move.end_square, move.captured_piece);
        }
    }

    ply_moves -= 1;
    if (this->turn_color == white)
        full_moves -= 1;
    change_turn();  // Changes turn color from white <-> black.
}

bool BoardState::operator==(const BoardState &other) const {
    if (!(board == other.board))
        return false;
    if (castling != other.castling)
        return false;
    if (turn_color != other.turn_color)
        return false;
    if (en_passant != other.en_passant)
        return false;
    if (en_passant && (en_passant_square != other.en_passant_square))
        return false;
    if (check != other.check)
        return false;
    if (ply_moves != other.ply_moves)
        return false;
    if (full_moves != other.full_moves)
        return false;
    return true;
}
void BoardState::Display_board() {
    const char *files = "A B C D E F G H";

    std::cout << "    " << files << std::endl;
    std::cout << "  +-----------------+" << std::endl;
    uint8_t ep_sq = err_val8;
    bool ep = en_passant;
    if (ep)
        ep_sq = en_passant_square;

    for (int r = 7; r >= 0; --r) {
        std::cout << r + 1 << " | ";
        for (int c = 0; c < 8; ++c) {
            uint8_t idx = NotationInterface::idx(r, c);
            Piece p = board.get_piece_at(idx);
            char printval = p.get_char();
            if (printval == ' ' && ep && ep_sq == idx)
                printval = 'x';
            std::cout << printval << ' ';
        }
        std::cout << "| " << r + 1 << "     ";
        switch (r) {
        case 7: {
            std::cout << "Turn: " << Piece::char_from_val(turn_color);
            break;
        }
        case 6: {
            std::cout << "Ply counter: " << ply_moves;
            break;
        }
        case 5: {
            std::cout << "Move counter: " << full_moves;
            break;
        }
        case 4: {
            std::cout << "En passant: " << en_passant;
            break;
        }
            // case 3: {
            //     std::cout << "Castle rights: " << NotationInterface::castling_rights(castling);
            //     break;
            // }
        case 2: {
            std::cout << "Number of white pieces: (not implemented)";
            break;
        }
        case 1: {
            std::cout << "Number of black pieces: (not implemented)";
            break;
        }
        }
        std::cout << std::endl;
    }

    std::cout << "  +-----------------+" << std::endl;
    std::cout << "    " << files << std::endl;
}
bool BoardState::read_fen(const std::string FEN) {
    this->reset();
    int row = 7;
    int col = 0;

    bool success = true;

    // ---------------------

    for (size_t i = 0; i < FEN.size(); ++i) {
        char ch = FEN[i];
        if (ch == '/')
            continue;
        if (ch == ' ') {
            // reached end of board part
            break;
        }

        if (std::isdigit(ch)) {
            int num = ch - '0';
            col += num;
        } else {
            Piece p = Piece(ch);
            if (!Piece::is_valid_piece(p)) {
                success = false;
                break;
            }

            int idx = NotationInterface::idx(row, col);
            board.add_piece(idx, p);
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
    if (turnPart == "w")
        turn_color = white;  // white
    else if (turnPart == "b")
        turn_color = black;  // black
    else
        success = false;

    // Castling rights
    castling = 0;
    if (castlePart != "-") {
        for (char c : castlePart) {
            switch (c) {
            case 'K':
                castling |= castling::cast_white_kingside;
                break;
            case 'Q':
                castling |= castling::cast_white_queenside;
                break;
            case 'k':
                castling |= castling::cast_black_kingside;
                break;
            case 'q':
                castling |= castling::cast_black_queenside;
                break;
            default:
                success = false;
            }
        }
    }

    // En passant
    if (epPart == "-") {
        en_passant = false;
    } else {
        en_passant = true;
        en_passant_square = NotationInterface::idx_from_string(epPart);
        if (en_passant_square > 63)
            success = false;
    }

    // Halfmove clock
    ply_moves = std::stoi(halfPart);

    // Fullmove number
    full_moves = std::stoi(movePart);

    // ----------------------------
    // 3. Store results in state
    // ----------------------------

    return success;
}
std::string BoardState::fen_from_state() const {
    std::string FEN;
    FEN.reserve(92);

    for (int row = 7; row >= 0; row--) {
        int emptyCount = 0;

        for (int col = 0; col <= 7; col++) {
            uint8_t idx = NotationInterface::idx(row, col);
            Piece piece = board.get_piece_at(idx);

            if (piece == none_piece) {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    FEN += std::to_string(emptyCount);
                    emptyCount = 0;
                }
                FEN += piece.get_char();
            }
        }

        if (emptyCount > 0)
            FEN += std::to_string(emptyCount);

        if (row != 0)
            FEN += '/';
    }

    FEN += ' ';

    // --------------------------------------------------------
    // 2. Whose turn?
    // --------------------------------------------------------
    FEN += (turn_color == white ? 'w' : 'b');
    FEN += ' ';

    // --------------------------------------------------------
    // 3. Castling rights
    // --------------------------------------------------------
    FEN += NotationInterface::castling_rights(castling);
    FEN += " ";

    // --------------------------------------------------------
    // 4. En passant square
    // --------------------------------------------------------
    if (en_passant)
        FEN += NotationInterface::string_from_idx(en_passant_square);
    else
        FEN += '-';

    FEN += ' ';

    // --------------------------------------------------------
    // 5. Halfmove (ply) clock
    // --------------------------------------------------------
    FEN += std::to_string(ply_moves);
    FEN += ' ';

    // --------------------------------------------------------
    // 6. Fullmove number
    // --------------------------------------------------------
    FEN += std::to_string(full_moves);

    return FEN;
}
void BoardState::print_piece_loc() const { board.print_piece_loc(); }
size_t BoardState::get_moves(std::array<Move, max_legal_moves> &moves) const {
    uint8_t num_moves = board.get_moves(moves, turn_color);
    return num_moves;
}
