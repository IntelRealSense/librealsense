// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

// Unit Test Goals:
// Test the timer utility classes: stopwatch, timer, periodic_timer.

//#cmake:add-file ../../../common/utilities/time/stopwatch.h


#include "common.h"
#include "../common/utilities/time/stopwatch.h"

using namespace utilities::time;

// Test description:
// Test the stopwatch main functions
TEST_CASE( "test stopwatch", "[stopwatch]" )
{
    stopwatch sw;

    // Verify constructor set the time.
    CHECK(sw.get_start().time_since_epoch().count() > 0);

    auto current_time = clock::now();

    // Verify calling order force time count order
    CHECK(sw.get_start() <= current_time);

    // Test elapsed() function
    CHECK(sw.get_elapsed() < TEST_DELTA_TIME);

    // Sleep for verifying progress of time
    std::this_thread::sleep_for(TEST_DELTA_TIME);

    // Check elapsed() function
    CHECK(sw.get_elapsed() > TEST_DELTA_TIME);

    // Check elapsed_ms() function
    CHECK(sw.get_elapsed_ms() > TEST_DELTA_TIME_MS);
    
    // Check reset() function
    sw.reset();

    // Verify reset cause the elapsed time to be smaller
    CHECK(sw.get_elapsed() < TEST_DELTA_TIME);
    CHECK(sw.get_elapsed_ms() < TEST_DELTA_TIME_MS);

}
