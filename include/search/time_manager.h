// Copyright 2025 Filip Agert
#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
using namespace std::chrono;
struct TimeControl {
    int wtime, btime, winc, binc;
    bool ponder = false;
    std::optional<int> movetime; // for hardcoding a time. if set, will think for this long.
};
struct TimeSettings {
    int buffer;
    int remtime_frac;
};
class TimeManager {
  private:
    std::atomic<bool> should_stop = false;                // Shared variable between threads.
    std::atomic<bool> should_start_next_iteration = true; // Shared variable between threads.
    std::atomic<bool> is_mate_search = false;
    std::atomic<bool> desperate_mate_search = true;
    std::mutex tm_mutex;
    std::thread timer_thread;
    int remtime, inc, enemy_remtime, enemy_inc, buffer, remtime_frac;
    std::atomic<bool> pondering;
    time_point<high_resolution_clock> starttime;
    time_point<high_resolution_clock> inittime;
    int calculate_time_elapsed_ms(bool use_inittime) const;
    void time_loop_function(int64_t target_move_time_ms);
    int64_t calculate_target_move_time_ms();
    void start_clock(std::optional<int64_t> t_target);

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
    bool get_should_start_new_iteration(int depth) const;

    /**
     * @brief Gets time elapsed at latest update of time clock.
     *
     * @return Time elapsed.
     */
    int get_time_elapsed() const;

    /**
     * @brief Called by e.g. calculation thread once it breaks from searching enough depth.
     *
     */
    void stop_and_join();

    /**
     * @brief Flag that we are currently performing a matesearch.
     *
     * @param desperate desperate: true -> we allow to spend more time to try to avoid mate (half remaining clock)
     * @return true
     * @return false
     */
    void set_matesearch_on(bool desperate);
    void set_matesearch_off();
    /**
     * @brief Do we have time to start another deepening iteration for finding shallower mate?
     *
     * @return true
     * @return false
     */
    bool has_mate_search_time();

    void ponderhit();

    bool is_pondering() const;

    TimeManager(TimeSettings settings);
    ~TimeManager();

    void init_search(TimeControl limits, bool is_white);

    void reset();
};
#endif
