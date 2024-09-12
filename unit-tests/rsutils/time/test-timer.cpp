// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:dependencies rsutils

// Unit Test Goals:
// Test the timer utility classes: stopwatch, timer, periodic_timer.

#include "common.h"
#include <rsutils/time/timer.h>

using namespace rsutils::time;

// Test description:
// > Test the timer main functions
// > Verify the timer expired only when the timeout is reached.
// > Verify restart process 
TEST_CASE( "test timer", "[timer]" )
{
    timer t(TEST_DELTA_TIME);

    CHECK_FALSE(t.has_expired());

    t.start();
    CHECK_FALSE(t.has_expired());

    std::this_thread::sleep_for(TEST_DELTA_TIME + std::chrono::milliseconds(100));

    // test has_expired() function - expect time expiration
    CHECK(t.has_expired());

    // test start() function and verify expiration behavior
    t.start();
    CHECK_FALSE(t.has_expired());

    std::this_thread::sleep_for(TEST_DELTA_TIME / 2);
    
    // Verify time has not expired yet
    CHECK_FALSE(t.has_expired());

    std::this_thread::sleep_for(TEST_DELTA_TIME);

    // Verify time expired
    CHECK(t.has_expired());

}

// Test description:
// Verify the we can force the time expiration
TEST_CASE("Test force time expiration", "[timer]")
{
    timer t(TEST_DELTA_TIME);

    CHECK_FALSE(t.has_expired());
    t.set_expired();
    CHECK(t.has_expired());
}
