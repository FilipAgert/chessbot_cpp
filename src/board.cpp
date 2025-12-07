// Copyright 2025 Filip Agert
#include <board.h>
#include <cassert>
#include <exceptions.h>
#include <iostream>
#include <movegen.h>
#include <sstream>
#include <vector>
using namespace movegen;

void Board::bb_move(const uint8_t from, const uint8_t to, const Piece p) {
    bit_boards[p.get_color()] &= ~bit_boards[p.get_value()];  // Clear color bitboard from piece
    uint64_t bb = BitBoard::one_high(from);
    bit_boards[p.get_value()] &= ~bb;
    bb = BitBoard::one_high(to);
    bit_boards[p.get_value()] |= bb;
    bit_boards[p.get_color()] |= bit_boards[p.get_value()];  // Set color bit board for this piece
}
void Board::bb_remove(const uint8_t sq, const Piece p) {
    bit_boards[p.get_color()] &= ~bit_boards[p.get_value()];  // Clear color bitboard from piece
    uint64_t bb = BitBoard::one_high(sq);
    bit_boards[p.get_value()] &= ~bb;
    bit_boards[p.get_color()] |= bit_boards[p.get_value()];  // Set color bit board for this piece
}
void Board::bb_add(const uint8_t sq, const Piece p) {
    bit_boards[p.get_color()] &= ~bit_boards[p.get_value()];  // Clear color bitboard from piece

    uint64_t bb = BitBoard::one_high(sq);
    bit_boards[p.get_value()] |= bb;

    bit_boards[p.get_color()] |= bit_boards[p.get_value()];  // Set color bit board for this piece
}

bool Board::is_square_empty(uint8_t square) const {
    return this->get_piece_at(square) == none_piece;
}

uint8_t Board::get_square_color(uint8_t square) const {
    return this->get_piece_at(square).get_color();
}

void Board::capture_piece(const uint8_t source, const uint8_t target) {
    bb_remove(target, game_board[target]);
    bb_move(source, target, game_board[source]);
    game_board[target] = game_board[source];
    game_board[source] = none_piece;
    num_pieces--;
}
void Board::capture_piece_ep(const uint8_t source, const uint8_t target,
                             uint8_t captured_pawn_loc) {
    bb_move(source, target, game_board[source]);                  // Move pawn
    bb_remove(captured_pawn_loc, game_board[captured_pawn_loc]);  // Remove captured pawn
    game_board[target] = game_board[source];
    game_board[source] = none_piece;
    game_board[captured_pawn_loc] = none_piece;
    num_pieces--;
}

void Board::move_piece(const uint8_t source, const uint8_t target) {
    bb_move(source, target, game_board[source]);
    game_board[target] = game_board[source];
    game_board[source] = none_piece;
}

void Board::add_piece(const uint8_t square, const Piece piece) {
    game_board[square] = piece;
    bb_add(square, piece);
    num_pieces++;
}

void Board::remove_piece(const uint8_t square) {
    bb_remove(square, game_board[square]);
    game_board[square] = none_piece;
    num_pieces--;
}
void Board::promote_piece(const uint8_t square, const Piece promotion) {
    bb_remove(square, game_board[square]);  // Remove pawn
    bb_add(square, promotion);
    game_board[square] = promotion;
}

void Board::clear_board() {
    for (size_t i = 0; i < 64; i++) {
        this->game_board[i] = Piece();
    }
    for (size_t i = 0; i < bit_boards.size(); i++) {
        this->bit_boards[i] = 0;
    }
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
uint64_t Board::to_squares(uint8_t ptype, uint8_t sq, uint64_t friendly_bb, uint64_t enemy_bb,
                           uint64_t ep_bb, uint8_t castleinfo, uint8_t turn_color) const {
    uint64_t piece_bb = BitBoard::one_high(sq);
    uint8_t enemy_col = turn_color ^ pieces::color_mask;
    uint64_t to_squares;
    switch (ptype) {
    case pieces::pawn:
        to_squares = movegen::pawn_moves(piece_bb, friendly_bb, enemy_bb, ep_bb, turn_color);
        break;
    case pieces::bishop:
        to_squares = movegen::bishop_moves_sq(sq, friendly_bb, enemy_bb);
        break;
    case pieces::knight:
        to_squares = movegen::knight_moves(sq, friendly_bb);
        break;
    case pieces::rook:
        to_squares = movegen::rook_moves_sq(sq, friendly_bb, enemy_bb);
        break;
    case pieces::queen:
        to_squares = movegen::queen_moves_sq(sq, friendly_bb, enemy_bb);
        break;
    case pieces::king:
        to_squares = movegen::king_moves(sq, friendly_bb, friendly_bb | enemy_bb,
                                         get_atk_bb(enemy_col), castleinfo, turn_color);
        break;
    }
    return to_squares;
}

/**
 * @brief Adds moves to movelist
 *
 * @param[inout] moves move array
 * @param[inout] num_moves number of moves. will be updated
 * @param[in] to_bb to squares in moves. is destroyed
 * @param[in] from from square
 */
void Board::add_moves(std::array<Move, max_legal_moves> &moves, size_t &num_moves, uint64_t &to_bb,
                      const uint8_t from) const {
    BitLoop(to_bb) {
        uint8_t lsb = BitBoard::lsb(to_bb);
        moves[num_moves++] = Move(from, lsb);
    }
}
void Board::add_moves_pawn(std::array<Move, max_legal_moves> &moves, size_t &num_moves,
                           uint64_t &to_bb, const uint8_t from, const uint8_t color) const {
    int promorow = (color == pieces::white) * 7;  // Row for pawn promotions.

    BitLoop(to_bb) {
        uint8_t lsb = BitBoard::lsb(to_bb);
        if (NotationInterface::row(lsb) == promorow) {
            for (uint8_t ptype : pieces::promote_types) {
                moves[num_moves++] = Move(from, lsb, Piece(ptype | color));
            }
        } else {
            moves[num_moves++] = Move(from, lsb);
        }
    }
}

void Board::gen_add_all_moves(std::array<Move, max_legal_moves> &moves, size_t &num_moves,
                              uint64_t &piece_bb, const uint8_t piecetype,
                              const uint64_t friendly_bb, const uint64_t enemy_bb,
                              const uint64_t ep_bb, const uint8_t castleinfo,
                              const uint8_t turn_color) const {
    BitLoop(piece_bb) {
        uint8_t sq = BitBoard::lsb(piece_bb);
        uint64_t to_sqs =
            to_squares(piecetype, sq, friendly_bb, enemy_bb, ep_bb, castleinfo, turn_color);
        if (piecetype == pieces::pawn) {
            add_moves_pawn(moves, num_moves, to_sqs, sq, turn_color);
        } else {
            add_moves(moves, num_moves, to_sqs, sq);
        }
    }
}
uint64_t Board::get_atk_bb(const uint8_t color) const {
    uint8_t enemy_color = color ^ pieces::color_mask;

    uint64_t friendly_pieces = bit_boards[color];
    uint64_t enemy_pieces = bit_boards[enemy_color];
    uint64_t occ = friendly_pieces | enemy_pieces;
    uint64_t queen_bb = bit_boards[color | pieces::queen];
    uint64_t bishop_bb = bit_boards[color | pieces::bishop] |
                         queen_bb;  // queen is taken care of through bishop call
    uint64_t rook_bb = bit_boards[color | pieces::rook] | queen_bb;  // and through rook call.
    uint64_t pawn_bb = bit_boards[color | pieces::pawn];
    uint64_t knight_bb = bit_boards[color | pieces::knight];
    uint64_t king_bb = bit_boards[color | pieces::king];
    bishop_bb = movegen::bishop_atk_bb(bishop_bb, occ);
    rook_bb = movegen::rook_atk_bb(rook_bb, occ);
    knight_bb = movegen::knight_atk_bb(knight_bb);
    pawn_bb = movegen::pawn_atk_bb(pawn_bb, color);
    king_bb = movegen::king_atk_bb(king_bb);
    return bishop_bb | rook_bb | knight_bb | pawn_bb | king_bb;
}

bool does_move_check(const Move candidate, const uint8_t king_color) {
    NotImplemented();
    // Can do a bitboard only implementation. Need to: Remove captured piece. Handle en passant.
}
bool Board::king_checked(const uint8_t turn_color) const {
    uint64_t king_bb = bit_boards[turn_color | pieces::king];
    uint8_t other_col = turn_color ^ pieces::color_mask;
    uint64_t enemy_atk_bb = get_atk_bb(other_col);
    return (king_bb & enemy_atk_bb) != 0;
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
void Board::do_move(Move &move) {
    move.castling_rights = this->castleinfo;
    move.en_passant_square = this->en_passant ? this->en_passant_square : err_val8;
    move.ply = ply_moves;
    ply_moves += 1;
    if (this->turn_color == black)
        full_moves += 1;
    change_turn();  // Changes turn color from white <-> black.
    Piece moved = get_piece_at(move.source);

    // Exception: en_passant.
    if (en_passant && moved.get_type() == pawn && move.target == en_passant_square) {
        move.captured = Piece(pawn | turn_color);  // Can only capture pawns in en_passant.
    } else {
        move.captured = get_piece_at(move.target);
    }

    // Exception: castle
    if (moved.get_type() == king &&
        abs(NotationInterface::col(move.target) - NotationInterface::col(move.source)) > 1) {
        // If kingside: (diff is positive)
        int diff = NotationInterface::col(move.target) - NotationInterface::col(move.source);

        uint8_t rook_from;
        uint8_t rook_to;
        if (diff > 0) {  // kingside
            rook_from = move.source + 3;
            rook_to = move.source + 1;
        } else {
            rook_from = move.source - 4;
            rook_to = move.source - 1;
        }
        move_piece(rook_from, rook_to);
    }
    // Set castling rights. If one of king squares moved -> remove castling right for that color.
    // If one of rooks moved or captured -> remove that sides castling rights.
    uint64_t from_to = BitBoard::one_high(move.source) | BitBoard::one_high(move.target);
    uint8_t castlemask =
        ((from_to & BitBoard::one_high(NotationInterface::idx_from_string("e1"))) != 0) *
            0b0011 |  // king w
        ((from_to & BitBoard::one_high(NotationInterface::idx_from_string("a1"))) != 0) *
            0b0010 |  // queenside w
        ((from_to & BitBoard::one_high(NotationInterface::idx_from_string("h1"))) != 0) *
            0b0001 |  // kingside w
        ((from_to & BitBoard::one_high(NotationInterface::idx_from_string("e8"))) != 0) *
            0b1100 |  // king b
        ((from_to & BitBoard::one_high(NotationInterface::idx_from_string("a8"))) != 0) *
            0b1000 |  // queenside b
        ((from_to & BitBoard::one_high(NotationInterface::idx_from_string("h8"))) != 0) *
            0b0100;  // kingside b
    this->castleinfo &= ~castlemask;

    if (move.captured.get_value() || moved.get_type() == pawn)
        ply_moves = 0;  // reset ply on move pawn or capture piece.

    if (move.captured.get_value()) {
        if (en_passant && moved.get_type() == pawn && move.target == en_passant_square) {
            uint8_t captured_pawn_loc = (turn_color == black) ? move.target - 8 : move.target + 8;
            // Depends on turn color. If black, we captured a black piece. This means that the
            // location of the pawn is target_square-8,else +8
            capture_piece_ep(move.source, move.target, captured_pawn_loc);
        } else {
            capture_piece(move.source, move.target);
        }
    } else {
        move_piece(move.source, move.target);
    }
    if (move.promotion.get_value()) {
        promote_piece(move.target, move.promotion);
        if (move.promotion.get_color() == none) {
            std::cerr << "Invalid argument: Promoted piece must have a color. Piece value: "
                      << move.promotion.get_value() << "\n";
            std::abort();
        }
    }
    // Set the en_passant flags
    if (moved.get_type() == pawn && abs(move.target - move.source) == 16) {
        en_passant = true;
        en_passant_square = (move.source + move.target) / 2;
    } else {
        en_passant = false;
        en_passant_square = err_val8;
    }
}

void Board::undo_move(const Move move) {
    castleinfo = move.castling_rights;
    en_passant_square = move.en_passant_square;
    en_passant = en_passant_square < 64;
    ply_moves = move.ply;
    move_piece(move.target, move.source);
    if (move.promotion.get_value()) {
        promote_piece(move.source,
                      Piece(move.promotion.get_color() | pawn));  // Replace with pawn.
    }

    Piece moved = get_piece_at(move.source);
    // Exception2: Castle.
    if (moved.get_type() == king &&
        abs(NotationInterface::col(move.target) - NotationInterface::col(move.source)) > 1) {
        // If kingside: (diff is positive)
        int diff = NotationInterface::col(move.target) - NotationInterface::col(move.source);

        uint8_t rook_from;
        uint8_t rook_to;
        if (diff > 0) {  // kingside
            rook_from = move.source + 3;
            rook_to = move.source + 1;
        } else {
            rook_from = move.source - 4;
            rook_to = move.source - 1;
        }
        move_piece(rook_to, rook_from);
    }

    if (move.captured.get_value()) {
        if (en_passant && get_piece_at(move.source).get_type() == pawn &&
            move.target == en_passant_square) {
            uint8_t captured_pawn_loc = (turn_color == black) ? move.target - 8 : move.target + 8;
            add_piece(captured_pawn_loc, move.captured);
        } else {
            add_piece(move.target, move.captured);
        }
    }

    if (this->turn_color == white)
        full_moves -= 1;
    change_turn();  // Changes turn color from white <-> black.
}

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
            add_piece(idx, p);
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
