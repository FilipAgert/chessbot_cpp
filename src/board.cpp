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
void Board::piece_loc_move(uint8_t from, uint8_t to) {
    uint8_t idx = 0;
    while (idx < num_pieces) {
        if (piece_locations[idx] == from) {
            piece_locations[idx] = to;
            return;
        }
        idx++;
    }
    std::cerr << "No piece found at square: " << static_cast<int>(from) << " "
              << NotationInterface::string_from_idx(from) << std::endl;
    std::cerr << "Attempted move: " << Move(from, to).toString() << std::endl;
    std::cerr << "Number of pieces: " << static_cast<int>(num_pieces) << std::endl;
    std::cerr << "Locations in piece_loc:" << std::endl;
    for (int i = 0; i < num_pieces; i++)
        std::cerr << NotationInterface::string_from_idx(piece_locations[i]) << " ";
    std::cerr << std::endl;
    std::abort();
}
void Board::piece_loc_remove(uint8_t sq) {
    uint8_t idx = 0;
    while (idx < num_pieces) {
        if (piece_locations[idx] == sq)
            break;
        idx++;
    }
    idx++;
    while (idx < num_pieces) {
        piece_locations[idx - 1] = piece_locations[idx];
        idx++;
    }
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
    piece_loc_remove(
        start_square);  // Counterintuative, but the captured piece already points to this location
    num_pieces--;
}
void Board::capture_piece_ep(const uint8_t start_square, const uint8_t end_square,
                             uint8_t captured_pawn_loc) {
    bb_move(start_square, end_square, game_board[start_square]);  // Move pawn
    bb_remove(captured_pawn_loc, game_board[captured_pawn_loc]);  // Remove captured pawn
    game_board[end_square] = game_board[start_square];
    game_board[start_square] = none_piece;
    game_board[captured_pawn_loc] = none_piece;
    piece_loc_move(start_square, end_square);
    piece_loc_remove(captured_pawn_loc);
    num_pieces--;
}

void Board::move_piece(const uint8_t start_square, const uint8_t end_square) {
    bb_move(start_square, end_square, game_board[start_square]);
    game_board[end_square] = game_board[start_square];
    game_board[start_square] = none_piece;
    piece_loc_move(start_square, end_square);
}

void Board::add_piece(const uint8_t square, const Piece piece) {
    game_board[square] = piece;
    bb_add(square, piece);
    piece_loc_add(square);
    num_pieces++;
}

void Board::remove_piece(const uint8_t square) {
    bb_remove(square, game_board[square]);
    game_board[square] = none_piece;
    piece_loc_remove(square);
    num_pieces--;
}
void Board::promote_piece(const uint8_t square, const Piece promotion) {
    bb_remove(square, game_board[square]);  // Remove pawn
    bb_add(square, promotion);
    game_board[square] = promotion;
}

void Board::clear_board() {
    piece_locations.fill(0);
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
    for (uint8_t i = 0; i < num_pieces; i++) {
        pieces.push_back(game_board[piece_locations[i]]);
    }
    return pieces;
}
std::vector<std::pair<Piece, uint8_t>> Board::get_piece_num_moves(uint8_t castleinfo,
                                                                  uint64_t ep_bb) {
    std::vector<std::pair<Piece, uint8_t>> piece_moves;
    uint64_t white_bb = bit_boards[pieces::white];
    uint64_t black_bb = bit_boards[pieces::black];
    uint64_t friendly, enemy;
    for (uint8_t i = 0; i < num_pieces; i++) {
        uint8_t sq = piece_locations[i];
        Piece p = game_board[sq];
        uint8_t pcol = p.get_color();
        uint64_t piece_bb = BitBoard::one_high(sq);
        friendly = (pcol == pieces::white) * white_bb + (pcol == pieces::black) * black_bb;
        enemy = (pcol == pieces::black) * white_bb + (pcol == pieces::white) * black_bb;

        uint64_t to_squares_bb =
            to_squares(p.get_type(), piece_bb, friendly, enemy, ep_bb, castleinfo, pcol);
        uint8_t move_cnt = BitBoard::bitcount(to_squares_bb);

        piece_moves.push_back({p, move_cnt});
    }
    return piece_moves;
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
size_t Board::get_pseudolegal_moves(std::array<Move, max_legal_moves> &moves,
                                    const uint8_t turn_color, const bool en_passant,
                                    const uint8_t en_passant_sq, const uint8_t castleinfo) const {
    // Iterate over the locations of the pieces. Get all their available moves, add into array and
    // return number of moves.
    // 1. Generate all possible movemaps for each piece
    // 2. For each movemap, check for collisions. Treat enemy/friendly differently. Exclude all
    // other moves.
    // 3. For these moves, check if they lead to king in check.
    //    Easily with attack bitmaps? We can have current attack bitmap (king cannot move into),
    //    and we can have a bitmap ignoring first piece (first piece seethrough). If King is ON this
    //    bitmap NOW, then a piece might be pinned. Need to check if we're moving the pinned piece.
    //    Move generation is likely easier with bitmap since we can just shift.
    size_t num_moves = 0;
    uint64_t friendly_bb = bit_boards[turn_color];
    uint8_t enemy_col = pieces::color_mask ^ turn_color;
    uint64_t enemy_bb = bit_boards[enemy_col];
    uint64_t en_passant_bb =
        en_passant * (BitBoard::one_high(en_passant_sq));  // 0 if no en_passant.
    int promorow = (turn_color == pieces::white) * 7;      // Row for pawn promotions.

    for (int i = 0; i < num_pieces; i++) {
        uint8_t square = piece_locations[i];
        Piece p = game_board[square];
        if (p.get_color() != turn_color)  // Cycle if not correct color.
            continue;
        uint64_t to_squares_bb = to_squares(p.get_type(), square, friendly_bb, enemy_bb,
                                            en_passant_bb, castleinfo, turn_color);
        while (to_squares_bb > 0) {
            uint8_t to_sq = BitBoard::lsb(to_squares_bb);  // Extract LSB loc.
            to_squares_bb = BitBoard::clear_lsb(to_squares_bb);
            // Handle promotion
            if (p.get_type() == pieces::pawn &&
                (masks::row(promorow) & BitBoard::one_high(to_sq)) > 0) {
                // If to_sq is in last row or first row (depending on col)
                // TODO: Add settings for promotion selection.
                for (uint8_t p = pieces::queen; p < pieces::pawn; p++) {
                    moves[num_moves++] = Move(square, to_sq, Piece(p | turn_color));
                }
            } else {
                moves[num_moves++] = Move(square, to_sq);
            }
        }
    }
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

void Board::print_piece_loc() const {
    for (int i = 0; i < num_pieces; i++)
        std::cout << NotationInterface::string_from_idx(piece_locations[i]) << " ";
    std::cout << std::endl;
}
