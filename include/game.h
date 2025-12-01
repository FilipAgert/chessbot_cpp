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

    /**
     * @brief Enter the game loop logic. This should be called when UCI command GO is received.
     *
     */
    void start_thinking();

    /**
     * @brief Gets the current best known move to send to GUI.
     *
     */
    std::string get_bestmove() const;
    /**
     * @brief Set game to start position.
     *
     */
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

    BoardState get_state() { return state; }

 private:
    std::stack<Move> move_stack;
    Move bestmove;
    BoardState state;
    Game() = default;
};
