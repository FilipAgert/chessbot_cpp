// Copyright 2025 Filip Agert
#include <board_state.h>
#include <constants.h>
#include <move.h>
#include <notation_interface.h>
#include <piece.h>

#include <stack>
#include <string>
class Game {
 public:
    static Game &instance() {
        static Game game_instance;
        return game_instance;
    }
    void start_game();
    void end_game();
    void reset_game();
    void make_move(Move move);
    void undo_move();
    /**
     * @brief Prints board to console. Uppercase pieces are white, lowercase black.
     *
     */
    void display_board() { state.Display_board(); }
    std::string get_fen() const;
    /**
     * @brief Set the game state from FEN. If not success, game state now undefined.
     *
     * @param FEN FEN string
     * @return true OK
     * @return false Game state undefined.
     */
    bool set_fen(std::string FEN);

 private:
    std::stack<Move> move_stack;
    BoardState state;
    Game() = default;
};
