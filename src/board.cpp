// Copyright 2025 Filip Agert
#include <board.h>
#include <cassert>
#include <exceptions.h>
#include <iostream>
#include <movegen.h>
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

void Board::capture_piece(const uint8_t start_square, const uint8_t end_square) {
    bb_remove(end_square, game_board[end_square]);
    bb_move(start_square, end_square, game_board[start_square]);
    game_board[end_square] = game_board[start_square];
    game_board[start_square] = none_piece;
    num_pieces--;
}
void Board::capture_piece_ep(const uint8_t start_square, const uint8_t end_square,
                             uint8_t captured_pawn_loc) {
    bb_move(start_square, end_square, game_board[start_square]);  // Move pawn
    bb_remove(captured_pawn_loc, game_board[captured_pawn_loc]);  // Remove captured pawn
    game_board[end_square] = game_board[start_square];
    game_board[start_square] = none_piece;
    game_board[captured_pawn_loc] = none_piece;
    num_pieces--;
}

void Board::move_piece(const uint8_t start_square, const uint8_t end_square) {
    bb_move(start_square, end_square, game_board[start_square]);
    game_board[end_square] = game_board[start_square];
    game_board[start_square] = none_piece;
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
std::vector<std::pair<Piece, uint8_t>> Board::get_piece_num_moves(uint8_t castleinfo,
                                                                  uint64_t ep_bb) {

    std::vector<std::pair<Piece, uint8_t>> piece_moves;
    NotImplemented();
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
// Piece loc independent way
size_t Board::get_pseudolegal_moves(std::array<Move, max_legal_moves> &moves, const uint8_t color,
                                    const bool en_passant, const uint8_t en_passant_sq,
                                    const uint8_t castleinfo) const {
    size_t num_moves = 0;
    uint8_t enemy_color = color ^ pieces::color_mask;
    uint64_t friendly_bb = bit_boards[color];
    uint64_t enemy_bb = bit_boards[enemy_color];
    uint64_t queen_bb = bit_boards[color | pieces::queen];
    uint64_t bishop_bb = bit_boards[color | pieces::bishop];
    uint64_t rook_bb = bit_boards[color | pieces::rook];
    uint64_t pawn_bb = bit_boards[color | pieces::pawn];
    uint64_t knight_bb = bit_boards[color | pieces::knight];
    uint64_t king_bb = bit_boards[color | pieces::king];
    uint64_t ep_bb = en_passant ? BitBoard::one_high(en_passant_sq) : 0;
    gen_add_all_moves(moves, num_moves, queen_bb, pieces::queen, friendly_bb, enemy_bb, ep_bb,
                      castleinfo, color);
    gen_add_all_moves(moves, num_moves, bishop_bb, pieces::bishop, friendly_bb, enemy_bb, ep_bb,
                      castleinfo, color);
    gen_add_all_moves(moves, num_moves, rook_bb, pieces::rook, friendly_bb, enemy_bb, ep_bb,
                      castleinfo, color);
    gen_add_all_moves(moves, num_moves, king_bb, pieces::king, friendly_bb, enemy_bb, ep_bb,
                      castleinfo, color);
    gen_add_all_moves(moves, num_moves, knight_bb, pieces::knight, friendly_bb, enemy_bb, ep_bb,
                      castleinfo, color);
    gen_add_all_moves(moves, num_moves, pawn_bb, pieces::pawn, friendly_bb, enemy_bb, ep_bb,
                      castleinfo, color);
    return num_moves;
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
    return true;
}

Board::Board() {
    for (int i = 0; i < 64; i++) {
        game_board[i] = Piece();
    }
}

Board::~Board() {}
