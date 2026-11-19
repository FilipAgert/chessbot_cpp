#pragma once
#include "core/board.h"
#include "core/move.h"
#include "core/zobrist.h"
#include "eval/eval.h"
#include "search/TT.h"
#include "search/history.h"
#include "search/info.h"
#include "search/moveorder.h"
#include "search/state_history.h"

#include "search/parameters.h"
#include "search/time_manager.h"
#include <array>
#include <atomic>
#include <functional>
#include <optional>
#include <stack>

struct SearchStatistics {
    int seldepth = 0;

    long int reductions_total = 0;
    long int reductions_cnt = 0;
    long int research_cnt = 0;

    long int moves_generated = 0;
    long int nodes_evaluated = 0;
    long int nodes_quiesence_evaluated = 0;
};

enum class NodeType : uint8_t { PV, CUT, ALL };

struct Limits {
    TimeControl time;
    std::optional<int> depth = {};
    bool ponder = false;
};

struct SearchResult {
    std::optional<Move> bestmove;
    std::optional<Move> pondermove;
    int score = 0;
    int depth = 0;
};

struct SearchSettings {
    bool enable_aspiration;
};

class Searcher {
  public:
    Searcher(Eval::Eval &, TranspositionTable &, ZobroistHasher &, TimeManager &, const std::atomic<bool> &stop, std::function<void(InfoMsg &&)> on_info,
             SearchSettings);
    SearchResult search(const Board &root, const StateStack &state_stack, const Limits &limits);
    void ponderhit();
    void new_game(); // clears move_history, killers; called on ucinewgame

  private:
    // Search methods
    template <bool is_white> SearchResult think_loop(const Limits &limits);

    template <bool is_white> std::pair<int, SearchNodeType> aspiration_search(int preveval, int depth);
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
    template <bool is_root, bool is_white, bool is_zw, bool is_matesearch = false, NodeType node_type>
    std::pair<int, SearchNodeType> alpha_beta(int depth, int ply, int alpha, int beta, int num_extensions);
    /**
     * @brief Quiesence search. Only evaluates captures
     * @param [in] ply - Number of ply moves deep. 0 is root node and counting up.
     * @param[in] alpha Maximum guaranteed score of maximising player.
     * @param[in] beta Minimum guaranteeds core of minimising player
     * @return Score of current state.
     */
    template <bool is_white, bool is_mate_search> std::pair<int, SearchNodeType> quiesence(int ply, int alpha, int beta);

    template <bool is_white> void make_nullmove();
    template <bool is_white> void make_move(Move move);
    template <bool is_white> void undo_move();

    void reset_killers();
    bool should_abort() const;
    int reduction(int depth, int i) const;
    void precompute_reduction();
    /**
     * @brief Computes the extension for this move
     *
     * @param[in] move move played
     * @param[in] Order of move in search.
     * @param[in] num_extensions number of extensions previously performed
     * @return move extensions
     */
    int calculate_extension(const Move move, uint8_t movenum, int num_extensions, bool enemy_king_checked) const;
    /**
     * @brief Returns true if this board state has occured already in the tree. This means that if
     * the zobroist hash is already in the game stack. It is okay with just one and not thrice,
     * since its easier and since the state is deterministic anyway.
     *
     * @return true if this position already occured.
     */
    bool is_repetition(int ply);

    bool is_draw(int ply);
    template <bool can_cutoff> TTProbeResult probe_tt(uint64_t hash, int depth, int ply, int alpha, int beta);
    // definitions
    static constexpr int MAX_PLY = 128;
    static constexpr int INF = 10000000;
    static constexpr int REDUCTION_DEPTH_CAP = 64;

    // Communicate with engine
    const std::atomic<bool> &stop_requested;
    std::function<void(InfoMsg &&)> on_info;

    // private members
    Eval::Eval &eval;
    TranspositionTable &tt;
    ZobroistHasher &hasher;
    TimeManager &time_manager;

    StateStack state_stack;
    Board board;
    History move_history;

    std::array<std::array<Move, max_legal_moves>, MAX_PLY> move_arr;
    std::array<std::array<MoveOrder::MoveScore, max_legal_moves>, MAX_PLY> move_heuristic_score;
    std::array<std::array<Move, Parameters::NUM_KILLER_MOVES>, MAX_PLY> killer_moves;
    std::array<std::array<int, max_legal_moves>, REDUCTION_DEPTH_CAP> reductions; // move reductions.
    std::stack<Move> move_stack;
    std::stack<RestoreMoveInfo> restore_info_stack;

    SearchStatistics stats = {};
    SearchSettings settings;

    // private flags and similar
    bool in_null = false;
    bool must_finish_search = false;
    Move root_best;
};