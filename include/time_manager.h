#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H
#include <atomic>

class TimeManager {
 private:
    std::atomic<bool> should_stop;

 public:
    /**
     * @brief Gets flag if calculation thread should stop or not.
     *
     * @return True if calculation should stop.
     */
    bool get_should_stop();

    void start_timer();
};
#endif
