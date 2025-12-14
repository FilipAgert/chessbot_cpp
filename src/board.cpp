// Copyright 2025 Filip Agert
#include <board.h>
#include <cassert>
#include <exceptions.h>
#include <iostream>
#include <movegen.h>
#include <sstream>
#include <vector>
using namespace movegen;
using namespace pieces;

bool Board::is_square_empty(uint8_t square) const {
    return this->get_piece_at(square) == none_piece;
}

uint8_t Board::get_square_color(uint8_t square) const {
    return this->get_piece_at(square).get_color();
}

void Board::clear_board() {
    for (size_t i = 0; i < 64; i++) {
        this->game_board[i] = Piece();
    }
    white_pieces = 0;
    white_queen = 0;
    white_king = 0;
    white_bishops = 0;
    white_knights = 0;
    white_rooks = 0;
    white_pawns = 0;
    black_pieces = 0;
    black_queen = 0;
    black_king = 0;
    black_bishops = 0;
    black_knights = 0;
    black_rooks = 0;
    black_pawns = 0;
    num_pieces = 0;
}
std::vector<Piece> Board::get_pieces() {
    std::vector<Piece> pieces;
    for (uint8_t color : pieces::colors) {
        for (uint8_t ptype : pieces::piece_types) {
            uint8_t pval = ptype | color;
            size_t count = BitBoard::bitcount(bit_boards[pval]);
            for (uint8_t i = 0; i < count; i++) {
                pieces.push_back(Piece(pval));
            }
        }
    }
    return pieces;
}

bool does_move_check(const Move candidate, const uint8_t king_color) {
    NotImplemented();
    // Can do a bitboard only implementation. Need to: Remove captured piece. Handle en passant.
}

bool Board::operator==(const Board &other) const {
    for (int i = 0; i < 64; i++)
        if (!(game_board[i] == other.game_board[i]))
            return false;
    if (num_pieces != other.num_pieces)
        return false;
    if (castleinfo != other.castleinfo)
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

Board::Board() {
    for (int i = 0; i < 64; i++) {
        game_board[i] = Piece();
    }
}

Board::~Board() {}
using namespace pieces;

void Board::Display_board() {
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
            Piece p = get_piece_at(idx);
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
bool Board::read_fen(const std::string FEN) {
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
            if (p.get_color() == pieces::white)
                add_piece<true>(idx, p);
            else
                add_piece<false>(idx, p);

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
    castleinfo = 0;
    if (castlePart != "-") {
        for (char c : castlePart) {
            switch (c) {
            case 'K':
                castleinfo |= castling::cast_white_kingside;
                break;
            case 'Q':
                castleinfo |= castling::cast_white_queenside;
                break;
            case 'k':
                castleinfo |= castling::cast_black_kingside;
                break;
            case 'q':
                castleinfo |= castling::cast_black_queenside;
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
std::string Board::fen_from_state() const {
    std::string FEN;
    FEN.reserve(92);

    for (int row = 7; row >= 0; row--) {
        int emptyCount = 0;

        for (int col = 0; col <= 7; col++) {
            uint8_t idx = NotationInterface::idx(row, col);
            Piece piece = get_piece_at(idx);

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
    FEN += NotationInterface::castling_rights(castleinfo);
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
