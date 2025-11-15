#include <piece.h>
#include <board_state.h>
#include <constants.h>
#include <notation_interface.h>

class Game {
public:
    Game();
    ~Game();
    void start_game();
    void end_game();
    void reset_game();
    void make_move(uint8_t start_square, uint8_t end_square);
    void undo_move();
    void display_board();
    std::string get_fen() const;
    void set_fen(std::string FEN);
private:
    BoardState state;
};
