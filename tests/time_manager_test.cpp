
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <time_manager.h>
TEST(TimeManagerTest, ontime) {
    int buffer = 50;
    int remtime = 1000;
    int timeinc = 50;
    int remtime_frac = 20;
    time_control rem_time = time_control(remtime, remtime, timeinc, timeinc);
    TimeManager manager = TimeManager(rem_time, buffer, remtime_frac, true);
    auto start = std::chrono::high_resolution_clock::now();
    int target = remtime / remtime_frac + timeinc - buffer;  // Here is where we want to go.

    auto sleeptime = 1ms;
    manager.start_time_management();
    for (int i = 0; i < target; i++) {
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        int durms = duration.count();

        // std::cout << "Duration: " << durms << " ms out of target: " << target
        //           << " ms. Time manager flag: " << manager.get_should_stop() << std::endl;
        //  Assert that it if we are more than 5% from target, its an error
        if (durms < target) {
            ASSERT_FALSE(manager.get_should_stop());
        }

        std::this_thread::sleep_for(sleeptime);
    }
    // 2ms margin.
    std::this_thread::sleep_for(sleeptime * 2);
    ASSERT_TRUE(manager.get_should_stop()) << "by now time manager should have stopped";
    manager.stop_and_join();
}
TEST(TimeManagerTest, thread_stop_test) {
    int buffer = 50;
    int remtime = 1000;
    int timeinc = 50;
    int remtime_frac = 20;
    time_control rem_time = time_control(remtime, timeinc, remtime, timeinc);
    TimeManager manager = TimeManager(rem_time, buffer, remtime_frac, true);

    auto start = std::chrono::high_resolution_clock::now();
    manager.start_time_management();
    manager.stop_and_join();
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    int durms = duration.count();
    ASSERT_LE(durms, 1);
}
