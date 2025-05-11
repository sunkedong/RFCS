//
// Created by melody on 24-11-5.
//

#include <iostream>
#include <cassert>
#include "muduo/net/Timer.h"
#include "muduo/base/Timestamp.h"

// Mock callback function for testing
void mockCallback() {
    std::cout << "Timer fired!" << std::endl;
}

void testTimer() {
    // Set up a Timer with a mock callback, current time, and an interval of 1 second
    muduo::Timestamp startTime = muduo::Timestamp::now();
    double interval = 1.0;  // Timer should repeat every 1 second
    muduo::net::Timer timer(mockCallback, startTime, interval);

    // Check that the expiration time is set correctly
    assert(timer.expiration() == startTime);

    // Check that repeat is enabled because interval > 0
    assert(timer.repeat() == true);

    // Check that the timer sequence number increments
    int64_t initialSequence = timer.sequence();
    assert(initialSequence == 1);

    // Run the callback function to simulate the timer firing
    timer.run();

    // Restart the timer and verify expiration time is updated
    timer.restart(muduo::Timestamp::now());
    assert(timer.expiration() > startTime);

    // Print to confirm test completion
    std::cout << "All tests passed!" << std::endl;
}

int main() {
    testTimer();
    return 0;
}
