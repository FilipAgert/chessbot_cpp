// Copyright 2025 Filip Agert
#include <board_state.h>
#include <constants.h>
#include <move.h>
#include <notation_interface.h>
#include <piece.h>

#include <stack>
#include <string>
#include <vector>

struct InfoMsg {
    int depth;     // half-ply
    int seldepth;  // Depth reached in quiesence
    int time;      // Time elapsed since start of search
    int nodes;     // Number of positions evaluated.
    int moves_generated;
    int nps;
    std::vector<Move> pv;  // Principal line
    Move currmove;         // Current move (root) being evaluated.
    int score;             // Current evaluated best move score
    int d0score;           // Score of state (no going deep)
};
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

    void eval_board(size_t depth);

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

    /**
     * @brief Reset the game info such as number of moves explored when starting a new search.
     *
     */
    void reset_infos();

 private:
    std::stack<InfoMsg> info_stack;
    std::stack<Move> move_stack;
    Move bestmove;
    BoardState state;
    Game() = default;
    /**
     * @brief Main game logic loop for thinking about a position.
     *
     */
    void think_loop();
};
