// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

// Unit Test Goals:
// Test the timer utility classes: stopwatch, timer, periodic_timer.

#include "../common_unit_tests_header.h"
#include "../common/utilities/timer.h"

using namespace time_utilities;

// Test description:
// Test the stopwatch main functions
TEST_CASE( "stopwatch", "[Timer]" )
{
    stopwatch sw;

    // Verify constructor set the time.
    CHECK(sw.get_start_time().time_since_epoch().count() > 0);

    auto current_time = sw.now();

    // Verify now() function set the time
    CHECK(current_time.time_since_epoch().count() > 0);

    // Verify calling order force time count order
    CHECK(sw.get_start_time() <= current_time);

    // Test elapsed() function
    CHECK(sw.elapsed() < std::chrono::milliseconds(200));

    // Sleep for verifying progress of time
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Check elapsed() function
    CHECK(sw.elapsed() > std::chrono::milliseconds(200));

    // Check elapsed_ms() function
    CHECK(sw.elapsed_ms() > 200);
    
    // Check reset() function
    sw.reset();

    // Verify reset cause the elapsed time to be smaller
    CHECK(sw.elapsed() < std::chrono::milliseconds(200));
    CHECK(sw.elapsed_ms() < 200);

}

// Test description:
// > Test the timer main functions
// > Verify the timer expired only when the timeout is reached.
// > Verify restart process 
TEST_CASE( "timer", "[Timer]" )
{
    timer t(std::chrono::seconds(1));

    CHECK_FALSE(t.has_expired());

    t.start();
    CHECK_FALSE(t.has_expired());

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    // test has_expired() function - expect time expiration
    CHECK(t.has_expired());

    // test start() function and verify expiration behavior
    t.start();
    CHECK_FALSE(t.has_expired());

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Verify time has not expired yet
    CHECK_FALSE(t.has_expired());

    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    // Verify time expired
    CHECK(t.has_expired());

    // Check signal function
    t.start();
    CHECK_FALSE(t.has_expired());
    t.signal();
    CHECK(t.has_expired());

}

// Test description:
// Test the periodic_timer main functions
// Verify the timer expired and restart itself

TEST_CASE( "periodic_timer", "[Timer]" )
{
    periodic_timer pt(std::chrono::milliseconds(200));
    for (auto i = 0; i < 5; ++i)
    {
        CHECK_FALSE(pt);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        CHECK(pt);
    }

    // Check signal function
    CHECK_FALSE(pt);
    pt.signal();
    CHECK(pt);
}
