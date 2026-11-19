// Copyright 2025 Filip Agert
#include "interface/engine.h"
#include "core/board.h"
#include "core/move.h"
#include "core/notation_interface.h"
#include "eval/eval.h"
#include "search/TT.h"
#include "search/time_manager.h"
#include "utils/config.h"
#include <climits> // For infinity
#include <cstdint>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
Engine::Engine(ConfigSettings settings, Eval::Eval &eval_)
    : eval(eval_), settings_(settings), tt(hasher, TT_SIZE_MB), time_manager({STANDARD_TIME_BUFFER, STANDARD_TIME_FRAC}) {
    start_game();
}

void Engine::set_settings(ConfigSettings settings) {
    settings_ = settings;
    abort_search_and_wait();
    build_searcher();
}

bool Engine::set_fen(std::string FEN) {
    bool success = board.read_fen(FEN);
    assert(board.board_BB_match());
    reset_state_stack();
    tt.reset_age();
    return success;
}
void Engine::build_searcher() {

    auto on_info = [this](InfoMsg &&msg) { this->push_msg(std::move(msg)); };

    SearchSettings search_settings = {settings_.aspiration_enable};
    searcher = std::make_unique<Searcher>(eval, tt, hasher, time_manager, stop_requested_, on_info, search_settings);
}

void Engine::start_game() {
    abort_search_and_wait();
    reset_state_stack();
    tt.clear();
    tt.reset_age();
    build_searcher();
}

void Engine::abort_search_and_wait() {
    if (search_thread_.joinable()) {
        stop_requested_ = true;
        search_thread_.join();
        search_finished = true;
    }
    stop_requested_ = false;
}
void Engine::start_thinking(const TimeControl &tc, std::optional<int> depth, bool ponder, bool assign_worker) {
    abort_search_and_wait();

    search_finished = false;
    Limits limits = {tc, depth, ponder};

    time_manager.stop_and_join();
    time_manager.init_search(tc, board.get_turn_color() == pieces::white);
    auto run_search = [this, limits = limits]() {
        SearchResult result = searcher->search(board, state_stack, limits);
        time_manager.stop_and_join();
        {
            std::lock_guard<std::mutex> lock(last_result_mutex);
            last_result = result;
        }
        search_finished = true;
    };
    if (assign_worker) {
        search_thread_ = std::jthread(std::move(run_search));
    } else {
        run_search();
    }
}
void Engine::reset_state_stack() {
    state_stack.reset();
    uint64_t board_hash = hasher.hash_board(board);
    state_stack.push({board_hash, board.get_num_pieces()});
}

InfoMsg Engine::info_queue_front() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    return info_queue.front();
}
void Engine::info_queue_pop() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    info_queue.pop();
}
bool Engine::info_queue_empty() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    return info_queue.empty();
}

Move Engine::get_bestmove() const {
    std::lock_guard<std::mutex> lock(last_result_mutex);
    return last_result.bestmove.value_or(Move());
}
Move Engine::get_pondermove() const {
    std::lock_guard<std::mutex> lock(last_result_mutex);
    return last_result.pondermove.value_or(Move());
}
void Engine::push_msg(InfoMsg &&msg) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    info_queue.push(std::move(msg));
}

void Engine::gui_make_move(Move move) {
    assert(move.is_valid());
    if (board.get_turn_color() == pieces::white)
        board.do_move_no_flag<true>(move);
    else
        board.do_move_no_flag<false>(move);
    tt.increment_age();

    uint64_t state_hash = hasher.hash_board(board);
    state_stack.push({state_hash, board.get_num_pieces()});
}
void Engine::set_startpos() { set_fen(NotationInterface::starting_FEN()); }

std::string Engine::get_fen() const { return board.fen_from_state(); }

void Engine::ponderhit() {
    if (searcher)
        searcher->ponderhit();
}