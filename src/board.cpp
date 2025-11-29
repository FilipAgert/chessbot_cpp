// Copyright 2025 Filip Agert
#include <board.h>
#include <iostream>
#include <movegen.h>
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
    uint8_t bb = BitBoard::one_high(sq);
    bit_boards[p.get_value()] &= ~bb;
    bit_boards[p.get_color()] |= bit_boards[p.get_value()];  // Set color bit board for this piece
}
void Board::bb_add(const uint8_t sq, const Piece p) {
    bit_boards[p.get_color()] &= ~bit_boards[p.get_value()];  // Clear color bitboard from piece

    uint8_t bb = BitBoard::one_high(sq);
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
        bit_boards[i] = 0;
    }
    num_pieces = 0;
}
size_t Board::get_moves(std::array<Move, max_legal_moves> &moves, const uint8_t turn_color) const {
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
    uint64_t enemy_bb = bit_boards[pieces::color_mask ^ turn_color];
    uint64_t en_passant_bb = 0;  // TODO: Set actual en_passant value somehow.

    for (int i = 0; i < num_pieces; i++) {
        uint8_t square = piece_locations[i];
        Piece p = game_board[square];
        if (p.get_color() != turn_color)  // Cycle if not correct color.
            continue;
        uint64_t bb = BitBoard::one_high(square);
        uint64_t to_squares;
        switch (p.get_type()) {
        case pieces::pawn:
            to_squares = movegen::pawn_moves(bb, friendly_bb, enemy_bb, en_passant_bb, turn_color);

            break;
        case pieces::bishop:
            to_squares = movegen::bishop_moves(bb, friendly_bb, enemy_bb);
            break;
        case pieces::knight:
            to_squares = movegen::knight_moves(bb, friendly_bb);
            break;
        case pieces::rook:
            to_squares = movegen::rook_moves(bb, friendly_bb, enemy_bb);
            break;
        case pieces::queen:
            to_squares = movegen::queen_moves(bb, friendly_bb, enemy_bb);
            break;
        case pieces::king:
            to_squares = movegen::king_moves(bb, friendly_bb);
            break;
        }
        //  From to_squares create list of moves.
        uint8_t to_sq = 0;
        while (to_squares > 0) {
            uint8_t to = BitBoard::lsb(to_squares);  // Extract LSB loc.
            to_sq += to;
            to_squares >>= (to + 1);  // Clear LSB
            // TODO: Handle promotions for pawns.
            moves[num_moves++] = Move(square, to_sq);
        }
    }
    return num_moves;
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
