// Copyright 2025 Filip Agert
#ifndef GAME_H
#define GAME_H
#include "core/board.h"
#include "core/move.h"
#include "core/zobrist.h"
#include "eval/eval.h"
#include "search/TT.h"
#include "search/searcher.h"
#include "search/state_history.h"
#include "search/time_manager.h"
#include "utils/config.h"
#include <memory>

#include <optional>
#include <queue>
#include <string>

class Engine {
  private:
    Eval::Eval &eval;
    ConfigSettings settings_;
    Board board;
    StateStack state_stack;
    ZobroistHasher hasher;
    TranspositionTable tt; // after hasher
    std::atomic<bool> stop_requested_{false};
    std::queue<InfoMsg> info_queue;
    mutable std::mutex queue_mutex;
    std::unique_ptr<Searcher> searcher; // after everything it references
    TimeManager time_manager;
    mutable std::mutex last_result_mutex;
    SearchResult last_result;
    std::jthread search_thread_; // LAST: destroyed first
    void push_msg(InfoMsg &&);
    void reset_state_stack();
    void build_searcher();

  public:
    Engine(ConfigSettings settings, Eval::Eval &eval);
    Engine() = delete;
    void set_settings(ConfigSettings settings);
    void start_game();
    void start_thinking(const TimeControl &tc, std::optional<int> depth, bool ponder, bool assign_worker = true);
    void abort_search_and_wait();
    void ponderhit();
    Move get_bestmove() const;
    Move get_pondermove() const;
    // position
    bool set_fen(std::string fen);
    void set_startpos();
    std::string get_fen() const;
    void gui_make_move(Move m);
    const Board &get_board() const { return board; }
    Board get_board() { return board; }
    void display_board() { board.Display_board(); }
    // info queue
    bool info_queue_empty();
    InfoMsg info_queue_front();
    void info_queue_pop();
    std::atomic<bool> search_finished{false};
};

#endif