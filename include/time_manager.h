// Copyright 2025 Filip Agert
#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H
#include <atomic>
#include <chrono>
#include <thread>
using namespace std::chrono;
struct time_control {
    int wtime, btime, winc, binc;
};
class TimeManager {
 private:
    std::atomic<bool> should_stop;                  // Shared variable between threads.
    std::atomic<bool> should_start_next_iteration;  // Shared variable between threads.
    std::thread timer_thread;
    int remtime, inc, enemy_remtime, enemy_inc, buffer, remtime_frac;
    bool infinite;
    time_point<high_resolution_clock> start;
    int calculate_time_elapsed_ms() const;
    void time_loop_function(int64_t target_move_time_ms);
    int64_t calculate_target_move_time_ms();
    void set_should_start_next_iteration(bool start_flag);

 public:
    /**
     * @brief Gets flag if calculation thread should stop or not.
     *
     * @return True if calculation should stop.
     */
    bool get_should_stop() const;

    /**
     * @brief Gets if we should start a new iteration. For this to be true, we need that >1/2 of
     * alloted time is remaining.
     *
     * @return True if we can start a new iteration, else false.
     */
    bool get_should_start_new_iteration() const;

    /**
     * @brief Gets time elapsed at latest update of time clock.
     *
     * @return Time elapsed.
     */
    int get_time_elapsed() const;

    /**
     * @brief Set the stop flag. If time class, this will stop game loop. If game loop class, this
     * will stop time thread.
     *
     * @param[in] stop_flag true if loops should stop.
     */
    void set_should_stop(bool stop_flag);

    void start_time_management();

    /**
     * @brief Called by e.g. calculation thread once it breaks from searching enough depth.
     *
     */
    void stop_and_join();

    TimeManager(time_control rem_time, int buffer, int remtime_frac, bool is_white);

    ~TimeManager();
};
#endif
