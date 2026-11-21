// Copyright 2025 Filip Agert
#include "search/time_manager.h"
#include <algorithm>
#include <atomic>
#include <ctime>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std::chrono;
bool TimeManager::get_should_stop() const { return should_stop.load(); }
bool TimeManager::is_pondering() const { return pondering; }
bool TimeManager::get_should_start_new_iteration(int depth) const { return (depth == 1) || should_start_next_iteration.load(); }
int TimeManager::get_time_elapsed() const { return calculate_time_elapsed_ms(true); } // return time_elapsed.load(); }
void TimeManager::set_matesearch_on(bool desperate) {
    desperate_mate_search = desperate;
    is_mate_search = true;
}
void TimeManager::set_matesearch_off() { is_mate_search = false; }

void TimeManager::start_clock(std::optional<int64_t> target) {
    if (timer_thread.joinable()) {
        should_stop = true;
        timer_thread.join();
    }
    should_stop = false;
    should_start_next_iteration = true;

    int64_t target_time = target.value_or(calculate_target_move_time_ms());
    starttime = high_resolution_clock::now();

    if (target_time != -1) {
        timer_thread = std::thread(&TimeManager::time_loop_function, this, target_time);
    }
}
void TimeManager::ponderhit() {
    std::lock_guard<std::mutex> lock(tm_mutex);
    if (!pondering.exchange(false))
        return;
    int64_t base_target = calculate_target_move_time_ms() / 2;
    start_clock(base_target); // we can afford to spend less time if we hit a pondermove.
}
int64_t TimeManager::calculate_target_move_time_ms() {
    int64_t target_time;
    if (pondering) {
        return -1;
    }

    int64_t base_time = remtime / remtime_frac; // Use up 1/20th of the remaining time plus increment.

    target_time = base_time + inc - buffer;
    // inc/2 to get some spare.

    int64_t mintime = 10;
    return std::max(target_time, mintime);
}
void TimeManager::time_loop_function(int64_t target_move_time_ms) {
    constexpr auto check_interval = 1ms; // Check every ms if calc thread stops us
    int64_t target_move_time = target_move_time_ms;
    bool adjusted = false;
    while (true) {
        if (get_should_stop()) {
            break;
        }
        int64_t elapsed_time = calculate_time_elapsed_ms(false);

        if (is_mate_search && !adjusted) {
            int64_t bonus = static_cast<int64_t>(static_cast<double>(target_move_time) * (desperate_mate_search ? 3.0 : 0.2));
            int64_t hard_cap = std::max(int64_t(1), static_cast<int64_t>(remtime) - 50);
            target_move_time = std::min(target_move_time + bonus, hard_cap);
            adjusted = true;
        }

        std::this_thread::sleep_for(check_interval);

        if (elapsed_time >= target_move_time) {
            should_stop = true;
            break;
        }
        if (elapsed_time >= target_move_time) {
            should_start_next_iteration = false;
        }
    }
}
// Called by the Search Thread (e.g., after it finds bestmove)
// or by the Main Thread (e.g., when receiving 'quit').
void TimeManager::stop_and_join() {
    // Ensure the stop flag is set so the time_loop_function will break its 'while(true)' loop.
    std::lock_guard<std::mutex> lock(tm_mutex);
    should_stop = true;
    should_start_next_iteration = false;

    // Wait for the thread to finish execution, if it's joinable.
    if (timer_thread.joinable()) {
        timer_thread.join();
    }
}

// Ensure the destructor also calls stop_and_join() for safety
TimeManager::~TimeManager() { stop_and_join(); }
int TimeManager::calculate_time_elapsed_ms(bool use_inittime) const {
    auto stop = high_resolution_clock::now();
    auto time = use_inittime ? inittime : starttime;
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - time);
    return duration.count();
}

TimeManager::TimeManager(TimeSettings settings) : buffer(settings.buffer), remtime_frac((settings.remtime_frac)) {}

void TimeManager::init_search(TimeControl limits, bool is_white) {
    std::lock_guard<std::mutex> lock(tm_mutex);
    if (timer_thread.joinable()) {
        should_stop = true;
        timer_thread.join();
    }
    reset();
    inittime = high_resolution_clock::now();
    remtime = is_white ? limits.wtime : limits.btime;
    enemy_remtime = is_white ? limits.btime : limits.wtime;
    inc = is_white ? limits.winc : limits.binc;
    enemy_inc = is_white ? limits.binc : limits.winc;
    pondering = limits.ponder;

    if (!pondering)
        start_clock(limits.movetime);
}

void TimeManager::reset() {
    should_stop = false;
    should_start_next_iteration = true;
    is_mate_search = false;
    desperate_mate_search = false;
}