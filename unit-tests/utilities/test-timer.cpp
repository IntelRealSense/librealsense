// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

// Unit Test Goals:
// Test the timer utility classes: stopwatch, timer, periodic_timer.

#include "../common_unit_tests_header.h"
#include "../common/utilities/timer.h"

using namespace time_utilities;

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
    t.set_expired();
    CHECK(t.has_expired());

}