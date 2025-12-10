// Copyright 2025 Filip Agert
#include <board.h>
#include <constants.h>
#include <memory>
#include <move.h>
#include <notation_interface.h>
#include <piece.h>
#include <tables.h>
#include <time_manager.h>

#include <queue>
#include <stack>
#include <string>
#include <vector>

struct InfoMsg {
    int depth = 0;              // half-ply
    int seldepth = 0;           // Depth reached in quiesence
    int time = 0;               // Time elapsed since start of search
    uint64_t nodes = 0;         // Number of positions evaluated.
    int moves_generated = 0;    // Number of total moves generated.
    std::vector<Move> pv = {};  // Principal line
    Move currmove = Move();     // Current move (root) being evaluated.
    int score = 0;              // Current evaluated best move score
    int d0score = 0;            // Score of state (no going deep)
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
     * @brief Returns true if this board state has occured already in the tree. This means that if
     * the zobroist hash is already in the game stack. It is okay with just one and not thrice,
     * since its easier and since the state is deterministic anyway.
     *
     * @return true if this position already occured.
     */
    bool check_repetition();
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
     * @param [in] ply - Number of ply moves deep. 0 is root node and counting up.
     * @param[[TODO:direction]] alpha Maximum guaranteed score of maximising player.
     * @param[[TODO:direction]] beta Minimum guaranteeds core of minimising player
     * @param[in] num_extensions - number of move extensions performed so far
     * @return Score of current state.
     */
    int alpha_beta(int depth, int ply, int alpha, int beta, int num_extensions);
    /**
     * @brief Quiesence search. Only evaluates captures
     * @param [in] ply - Number of ply moves deep. 0 is root node and counting up.
     * @param[in] alpha Maximum guaranteed score of maximising player.
     * @param[in] beta Minimum guaranteeds core of minimising player
     * @return Score of current state.
     */
    int quiesence(int ply, int alpha, int beta);

    /**
     * @brief Computes the extension for this move
     *
     * @param[in] move move played
     * @param[in] board board state
     * @param[in] num_extensions number of extensions previously performed
     * @return move extensions
     */
    int calculate_extension(const Move &move, int num_extensions) const;

    /**
     * @brief Gets the current best known move to send to GUI.
     *
     */
    Move get_bestmove() const;
    void make_move(Move move);
    void undo_move();
    /**
     * @brief Prints board to console. Uppercase pieces are white, lowercase black.
     *
     */
    void display_board() { board.Display_board(); }
    std::string get_fen() const;
    /**
     * @brief Set the game state from FEN. If not success, game state now undefined.
     *
     * @param FEN FEN string
     * @return true OK
     * @return false Game state undefined.
     */
    bool set_fen(std::string FEN);

    void set_startpos() { set_fen(NotationInterface::starting_FEN()); }
    Board get_board() { return board; }

    /**
     * @brief Reset the game info such as number of moves explored when starting a new search.
     *
     */
    void reset_infos();

    std::queue<InfoMsg> info_queue;

 private:
    std::stack<Move> move_stack;
    /**
     * @brief Clear stack and pushes current board state onto it.
     *
     */
    void reset_state_stack();
    StateStack state_stack;
    Move bestmove;
    Board board;
    uint64_t moves_generated;
    uint64_t nodes_evaluated;
    std::shared_ptr<TimeManager> time_manager;
    Game() = default;
    static constexpr int INF = 10000000;
    std::unique_ptr<transposition_table> trans_table = std::make_unique<transposition_table>();
    /**
     * @brief Main game logic loop for thinking about a position.
     *
     */
    bool one_depth_complete;
    void think_loop(const time_control rem_time);
};
