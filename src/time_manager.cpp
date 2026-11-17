// Copyright 2025 Filip Agert
#include <algorithm>
#include <atomic>
#include <iostream>
#include <thread>
#include <time_manager.h>

using namespace std::chrono;
bool TimeManager::get_should_stop() const {
    if (enable_tc)
        return should_stop.load();
    else
        return false;
}
bool TimeManager::is_pondering() const { return pondering; }
bool TimeManager::get_should_start_new_iteration() const {
    if (enable_tc)
        return should_start_next_iteration.load();
    else
        return true;
}
void TimeManager::set_should_stop(bool stop_flag) { should_stop.store(stop_flag); }
void TimeManager::set_should_start_next_iteration(bool start_flag) { should_start_next_iteration.store(start_flag); }
int TimeManager::get_time_elapsed() const { return calculate_time_elapsed_ms(); }  // return time_elapsed.load(); }
void TimeManager::set_matesearch_on(bool desperate) {
    desperate_mate_search = desperate;
    is_mate_search = true;
}
void TimeManager::set_matesearch_off() { is_mate_search = false; }

void TimeManager::start_time_management() {
    int64_t target_time = calculate_target_move_time_ms();
    start = high_resolution_clock::now();

    if (enable_tc && target_time != -1) {
        timer_thread = std::thread(&TimeManager::time_loop_function, this, target_time);
    }
}
void TimeManager::ponderhit() {
    pondering = false;
    start_time_management();
}
int64_t TimeManager::calculate_target_move_time_ms() {
    int64_t target_time;
    if (pondering) {
        return -1;
    }

    int64_t base_time = remtime / remtime_frac;  // Use up 1/20th of the remaining time plus increment.

    target_time = base_time + inc - buffer;

    int64_t mintime = 10;
    return std::max(target_time, mintime);
}
void TimeManager::time_loop_function(int64_t target_move_time_ms) {
    constexpr auto check_interval = 1ms;  // Check every ms if calc thread stops us
    int64_t target_move_time = target_move_time_ms;
    bool adjusted = false;
    while (true) {
        if (get_should_stop()) {
            break;
        }
        int64_t elapsed_time = calculate_time_elapsed_ms();

        if (is_mate_search & !adjusted) {
            int64_t bonus = static_cast<int64_t>(static_cast<double>(target_move_time) * (desperate_mate_search ? 3.0 : 0.2));
            int64_t hard_cap = std::max(int64_t(1), static_cast<int64_t>(remtime) - 50);
            target_move_time = std::min(target_move_time + bonus, hard_cap);
            adjusted = true;
        }

        std::this_thread::sleep_for(check_interval);

        if (elapsed_time >= target_move_time) {
            set_should_stop(true);
            break;
        }
        if (elapsed_time >= target_move_time / 2) {
            set_should_start_next_iteration(false);
        }
    }
}
// Called by the Search Thread (e.g., after it finds bestmove)
// or by the Main Thread (e.g., when receiving 'quit').
void TimeManager::stop_and_join() {
    // Ensure the stop flag is set so the time_loop_function will break its 'while(true)' loop.
    set_should_stop(true);
    set_should_start_next_iteration(false);

    // Wait for the thread to finish execution, if it's joinable.
    if (timer_thread.joinable()) {
        timer_thread.join();
    }
}

// Ensure the destructor also calls stop_and_join() for safety
TimeManager::~TimeManager() { stop_and_join(); }
int TimeManager::calculate_time_elapsed_ms() const {
    auto stop = high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    return duration.count();
}

TimeManager::TimeManager(TimeControl rem_time, int buffer_, int remtime_frac_, bool is_white, bool enable_tc_) : enable_tc(enable_tc_) {
    start = high_resolution_clock::now();
    remtime = is_white ? rem_time.wtime : rem_time.btime;
    enemy_remtime = is_white ? rem_time.btime : rem_time.wtime;
    inc = is_white ? rem_time.winc : rem_time.binc;
    enemy_inc = is_white ? rem_time.binc : rem_time.winc;
    pondering = rem_time.ponder;
    buffer = buffer_;
    remtime_frac = remtime_frac_;
    is_mate_search = false;
    set_should_start_next_iteration(true);
    set_should_stop(false);
}
