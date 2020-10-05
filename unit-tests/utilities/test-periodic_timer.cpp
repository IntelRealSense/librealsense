// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

// Unit Test Goals:
// Test the timer utility classes: stopwatch, timer, periodic_timer.

#include "../common_unit_tests_header.h"
#include "../common/utilities/periodic_timer.h"

using namespace time_utilities;

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
    pt.set_expired();
    CHECK(pt);
}
