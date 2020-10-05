// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

// Unit Test Goals:
// Test the timer utility classes: stopwatch, timer, periodic_timer.

#include "../common_unit_tests_header.h"
#include "../common/utilities/stopwatch.h"

using namespace time_utilities;

// Test description:
// Test the stopwatch main functions
TEST_CASE( "stopwatch", "[Timer]" )
{
    stopwatch sw;

    // Verify constructor set the time.
    CHECK(sw.get_start().time_since_epoch().count() > 0);

    auto current_time = stopwatch::clock::now();

    // Verify now() function set the time
    CHECK(current_time.time_since_epoch().count() > 0);

    // Verify calling order force time count order
    CHECK(sw.get_start() <= current_time);

    // Test elapsed() function
    CHECK(sw.get_elapsed() < std::chrono::milliseconds(200));

    // Sleep for verifying progress of time
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Check elapsed() function
    CHECK(sw.get_elapsed() > std::chrono::milliseconds(200));

    // Check elapsed_ms() function
    CHECK(sw.get_elapsed_ms() > 200);
    
    // Check reset() function
    sw.reset();

    // Verify reset cause the elapsed time to be smaller
    CHECK(sw.get_elapsed() < std::chrono::milliseconds(200));
    CHECK(sw.get_elapsed_ms() < 200);

}
