
#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
#include <thread>
#include <time_manager.h>
TEST(TimeManagerTest, ontime_5percent_margin) {
    int buffer = 50;
    int remtime = 1000;
    int timeinc = 50;
    int remtime_frac = 20;
    TimeManager manager = TimeManager(remtime, timeinc, remtime, timeinc, buffer, remtime_frac);
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
        if (durms < 0.95 * target) {
            ASSERT_FALSE(manager.get_should_stop());
        }

        std::this_thread::sleep_for(sleeptime);
    }
    // Give 5% margin until thread must signal to stop.
    for (int i = 0; i < target * 0.05; i++) {
        std::this_thread::sleep_for(sleeptime);
    }
    ASSERT_TRUE(manager.get_should_stop());
}
