// Copyright 2025 Filip Agert
#include <algorithm>
#include <atomic>
#include <iostream>
#include <thread>
#include <time_manager.h>

using namespace std::chrono;
bool TimeManager::get_should_stop() const { return should_stop.load(); }
void TimeManager::set_should_stop(bool stop_flag) { should_stop.store(stop_flag); }
int TimeManager::get_time_elapsed() const {
    return calculate_time_elapsed_ms();
}  // return time_elapsed.load(); }

void TimeManager::start_time_management() {
    int64_t target_time = calculate_target_move_time_ms();
    this->start = high_resolution_clock::now();

    if (target_time != -1) {
        this->timer_thread = std::thread(&TimeManager::time_loop_function, this, target_time);
    }
}
int64_t TimeManager::calculate_target_move_time_ms() {
    int64_t target_time;
    if (this->infinite) {
        return -1;
    } else {
        int64_t base_time =
            remtime / this->remtime_frac;  // Use up 1/20th of the remaining time plus increment.

        target_time = base_time + inc - buffer;  // With a 50 ms buffer.
    }

    int64_t mintime = 2;
    return std::max(target_time, mintime);
}
void TimeManager::time_loop_function(int64_t target_move_time_ms) {
    constexpr auto check_interval = 1ms;  // Check every 10 ms if calc thread stops us
    //

    while (true) {
        if (this->get_should_stop()) {
            break;
        }

        std::this_thread::sleep_for(check_interval);

        int64_t elapsed_time = calculate_time_elapsed_ms();

        if (elapsed_time >= target_move_time_ms) {
            this->set_should_stop(true);
            break;
        }
    }
}
// Called by the Search Thread (e.g., after it finds bestmove)
// or by the Main Thread (e.g., when receiving 'quit').
void TimeManager::stop_and_join() {
    // Ensure the stop flag is set so the time_loop_function will break its 'while(true)' loop.
    this->set_should_stop(true);

    // Wait for the thread to finish execution, if it's joinable.
    if (timer_thread.joinable()) {
        timer_thread.join();
    }
}

// Ensure the destructor also calls stop_and_join() for safety
TimeManager::~TimeManager() { stop_and_join(); }
int TimeManager::calculate_time_elapsed_ms() const {
    auto stop = high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - this->start);
    return duration.count();
}

TimeManager::TimeManager(time_control rem_time, int buffer, int remtime_frac, bool is_white) {
    this->remtime = is_white ? rem_time.wtime : rem_time.btime;
    this->enemy_remtime = is_white ? rem_time.btime : rem_time.wtime;
    this->inc = is_white ? rem_time.winc : rem_time.binc;
    this->enemy_inc = is_white ? rem_time.binc : rem_time.winc;
    this->infinite = false;
    this->buffer = buffer;
    this->remtime_frac = remtime_frac;
    this->set_should_stop(false);
}
