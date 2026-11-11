// Copyright 2025 Filip Agert
#ifndef GAME_H
#define GAME_H
#include "config.h"
#include "eval.h"
#include "integer_representation.h"
#include "moveorder.h"
#include "parameters.h"
#include <board.h>
#include <constants.h>
#include <history.h>
#include <memory>
#include <move.h>
#include <notation_interface.h>
#include <piece.h>
#include <tables.h>
#include <time_manager.h>

#include <queue>
#include <stack>
#include <string>
#include <utility>
#include <vector>
namespace NodeType {
constexpr uint8_t PV = 0;
constexpr uint8_t CUT = 1;
constexpr uint8_t ALL = 2;
}  // namespace NodeType
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
    int hashfill = 0;
    bool stringmsg = false;
    std::string string;
    float reduction_average;
    float reduction_research;
    int reduction_times;
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
    void start_thinking(const TimeControl rem_time);

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
     * @return Pair of the eval and bound type.
     */
    template <bool is_root, bool is_white, bool is_zw, uint8_t node_type>
    std::pair<int, uint8_t> alpha_beta(int depth, int ply, int alpha, int beta, int num_extensions);
    /**
     * @brief Quiesence search. Only evaluates captures
     * @param [in] ply - Number of ply moves deep. 0 is root node and counting up.
     * @param[in] alpha Maximum guaranteed score of maximising player.
     * @param[in] beta Minimum guaranteeds core of minimising player
     * @return Score of current state.
     */
    template <bool is_white> std::pair<int, uint8_t> quiesence(int ply, int alpha, int beta);

    /**
     * @brief Computes the extension for this move
     *
     * @param[in] move move played
     * @param[in] board board state
     * @param[in] num_extensions number of extensions previously performed
     * @return move extensions
     */
    int calculate_extension(const Move move, uint8_t movenum, int num_extensions, bool enemy_king_checked) const;

    /**
     * @brief Gets the current best known move to send to GUI.
     *
     */
    Move get_bestmove() const;
    template <bool is_white> void make_move_no_flag(Move move) {
        RestoreMoveInfo info = board.do_move_no_flag<is_white>(move);
        move_stack.push(move);
        restore_info_stack.push(info);
        uint64_t state_hash = ZobroistHasher::get().hash_board(board);
        state_stack.push(state_hash);
    }
    void make_move(Move move) {
        assert(move.is_valid());
        if (board.get_turn_color() == pieces::white)
            make_move_no_flag<true>(move);
        else
            make_move_no_flag<false>(move);
    }
    /**
     * @brief Assumes move is flagged.
     *
     * @tparam is_white true if white
     * @param[in] move move
     */
    template <bool is_white> void make_nullmove();
    template <bool is_white> void make_move(Move move);
    template <bool is_white> void undo_move();
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

    void set_settings(ConfigSettings settings) {
        settings_ = settings;
        precompute();
    }

    void precompute();
    void info_queue_pop();
    InfoMsg info_queue_front();
    bool info_queue_empty();
    void abort_search_and_wait();
    std::atomic<bool> search_finished{false};

    EvalState eval = EvalState();

 private:
    std::atomic<bool> stop_requested_{false};
    std::jthread search_thread_;

    std::queue<InfoMsg> info_queue;
    void push_msg(InfoMsg &&msg);
    mutable std::mutex queue_mutex;

    ConfigSettings settings_;
    static constexpr int MAX_PLY = 128;
    std::array<std::array<Move, max_legal_moves>, MAX_PLY> move_arr;
    std::array<std::array<MoveOrder::MoveScore, max_legal_moves>, MAX_PLY> move_heuristic_score;
    std::array<std::array<Move, NUM_KILLER_MOVES>, MAX_PLY> killer_moves;
    std::stack<Move> move_stack;
    std::stack<RestoreMoveInfo> restore_info_stack;
    History history;
    /**
     * @brief Clear stack and pushes current board state onto it.
     *
     */
    void reset_state_stack();
    StateStack state_stack;
    mutable std::mutex bestmove_mutex;
    Move bestmove;
    void set_bestmove(Move move);
    Board board;
    uint64_t moves_generated;
    uint64_t nodes_evaluated;
    uint64_t nodes_quiesence_evaluated;
    int seldepth = 0;
    std::shared_ptr<TimeManager> time_manager;
    Game() = default;
    Move root_best = Move();
    static constexpr int INF = 10000000;
    bool must_finish;  // if must finish this search or not.
    // table needs to be a pointer since its so large in memory.
    TranspositionTable trans_table = TranspositionTable(TT_SIZE_MB);

    long int reductions_total;
    long int reductions_cnt;
    long int research_cnt;

    bool in_null = false;  // if in null-move or not.

    void reset_killers();
    /**
     * @brief Main game logic loop for thinking about a position.
     *
     */
    inline bool should_abort() const { return stop_requested_ || (!must_finish && time_manager->get_should_stop()); }

    template <bool is_white> void think_loop(const TimeControl rem_time);

    /**
     * @brief Launches an aspiration search and handles the aspiration window
     *
     * @param[in] preveval previous iteration's evaluation.
     * @param[in] depth Depth of search.
     */
    template <bool is_white> std::pair<int, uint8_t> aspiration_search(int preveval, int depth);

    template <bool is_white> bool delta_futility_pruning(int standing_pat, int alpha) const {
        const EvalParameters &params = eval.params;
        const int queen = params.piecevals[pieces::queen];
        int big_delta = queen + Parameters::DELTA_THRESHHOLD;
        if (board.has_pawn_promotion<is_white>()) {
            big_delta += queen - params.piecevals[pieces::pawn];
        }
        return standing_pat + big_delta < alpha;
    }
    constexpr static int reduction_depth_cap = 64;
    std::array<std::array<int, max_legal_moves>, reduction_depth_cap> reductions;  // move reductions.
    int reduction(int depth, int i) const;
    void precompute_reduction();
};

#endif