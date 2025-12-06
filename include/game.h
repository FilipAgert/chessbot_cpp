// Copyright 2025 Filip Agert
#include <board_state.h>
#include <constants.h>
#include <move.h>
#include <notation_interface.h>
#include <piece.h>
#include <time_manager.h>

#include <queue>
#include <stack>
#include <string>
#include <vector>

struct InfoMsg {
    int depth;             // half-ply
    int seldepth;          // Depth reached in quiesence
    int time;              // Time elapsed since start of search
    uint64_t nodes;        // Number of positions evaluated.
    int moves_generated;   // Number of total moves generated.
    std::vector<Move> pv;  // Principal line
    Move currmove;         // Current move (root) being evaluated.
    int score;             // Current evaluated best move score
    int d0score;           // Score of state (no going deep)
    int mate;              // Number of moves to mate.
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
     * @param rem_time: Time control structure from input.
     *
     */
    void start_thinking(const time_control rem_time);

    /**
     * @brief Alpha beta pruning. This is quite complicated. alpha is the maximal guaranteed score
     * that the maximising player (white) can achive. Beta is the minimum guaranteed score that the
     * minimising player (black) can achive. This results in searching fewer branches. The reason is
     * that if the minimising player can GUARANTEE a score lower than the maximum score the
     * maximising player can achieve, then there is not point of searching this branch.
     *
     *
     * @param[in] depth Depth to go to. Breaks when depth is 0.
     * @param[[TODO:direction]] alpha Maximum guaranteed score of maximising player.
     * @param[[TODO:direction]] beta Minimum guaranteeds core of minimising player
     * @param[[TODO:direction]] is_maximiser If maximiser.
     * @return Score of current state.
     */
    int alpha_beta(size_t depth, int alpha, int beta);

    /**
     * @brief Gets the current best known move to send to GUI.
     *
     */
    Move get_bestmove() const;
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

    std::queue<InfoMsg> info_queue;

 private:
    std::stack<Move> move_stack;
    Move bestmove;
    BoardState state;
    uint64_t moves_generated;
    uint64_t nodes_evaluated;
    Game() = default;
    /**
     * @brief Main game logic loop for thinking about a position.
     *
     */
    void think_loop(const time_control rem_time);
};
