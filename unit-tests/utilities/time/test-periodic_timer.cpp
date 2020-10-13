// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

// Unit Test Goals:
// Test the timer utility classes: stopwatch, timer, periodic_timer.

//#cmake:add-file ../../../common/utilities/time/periodic_timer.h

#include "common.h"
#include "../common/utilities/time/periodic_timer.h"

using namespace utilities::time;

// Test description:
// Verify the timer expired and restart itself

TEST_CASE( "Test periodic time expiration", "[periodic_timer]" )
{
    periodic_timer pt(TEST_DELTA_TIME);
    for (auto i = 0; i < 5; ++i)
    {
        CHECK_FALSE(pt);
        std::this_thread::sleep_for(TEST_DELTA_TIME);
        CHECK(pt);
    }

    CHECK_FALSE(pt);
}

// Test description:
// Verify the we can force the time expiration
TEST_CASE("Test force time expiration", "[periodic_timer]")
{
    periodic_timer pt(TEST_DELTA_TIME);

    CHECK_FALSE(pt);
    pt.set_expired();
    CHECK(pt);
}
