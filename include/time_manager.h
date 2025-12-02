// Copyright 2025 Filip Agert
#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H
#include <atomic>
#include <chrono>
#include <thread>
using namespace std::chrono;
class TimeManager {
 private:
    std::atomic<bool> should_stop;  // Shared variable between threads.
    std::thread timer_thread;
    int remtime, inc, enemy_remtime, enemy_inc, buffer, remtime_frac;
    bool infinite;
    time_point<high_resolution_clock> start;
    int calculate_time_elapsed_ms();
    void time_loop_function(int64_t target_move_time_ms);
    int64_t calculate_target_move_time_ms();

 public:
    /**
     * @brief Gets flag if calculation thread should stop or not.
     *
     * @return True if calculation should stop.
     */
    bool get_should_stop() const;

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

    TimeManager(int remtime, int inc, int enemy_remtime, int enemy_inc, int buffer,
                int remtime_frac);
    explicit TimeManager(bool infinite);

    ~TimeManager();
};
#endif
