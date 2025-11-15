#include <piece.h>
#include <board.h>
#include <constants.h>


class game {
public:
    game();
    ~game();
    void start_game();
    void end_game();
    void reset_game();
    void make_move(uint8_t start_square, uint8_t end_square);
    void undo_move();
    void display_board();
    std::string get_fen() const;

private:
    Board game_board;
    uint8_t current_turn; // 0 for white, 1 for black
    int move_count;
    int ply_count;
    uint8_t castle_rights;
};
