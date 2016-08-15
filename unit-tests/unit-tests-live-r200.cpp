// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////
// This set of tests is valid only for the R200 camera //
/////////////////////////////////////////////////////////

#if !defined(MAKEFILE) || ( defined(LIVE_TEST) && defined(R200_TEST) )

#include "catch/catch.hpp"

#include "unit-tests-live-ds-common.h"

#include <climits>
#include <sstream>

/////////////////////
// Streaming tests //
/////////////////////

TEST_CASE("R200 devices support required options", "[live] [DS-device]")
{
    // Require at least one device to be plugged in
    safe_context ctx;
    const int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count > 0);

    // For each device
    for (int i = 0; i<device_count; ++i)
    {
        rs_device * dev = rs_get_device(ctx, 0, require_no_error());
        REQUIRE(dev != nullptr);

        rs_set_device_option(dev, RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED, 1.0, require_no_error());

        SECTION("R200 supports DS-Line standard UVC controls, and nothing else")
        {
            for (int i = 0; i<=RS_OPTION_COUNT; ++i)
            {
                auto x = rs_device_supports_option(dev, (rs_option)i, require_no_error());

                if ((i >= RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED && i <= RS_OPTION_R200_DEPTH_CONTROL_LR_THRESHOLD) ||
                    (i == RS_OPTION_FRAMES_QUEUE_SIZE))
                {
                    INFO("rs_option " << rs_option_to_string((rs_option)i) << " expected to be supported!")
                    REQUIRE(x == 1);
                }
                else
                {
                    INFO("rs_option " << rs_option_to_string((rs_option)i) << " expected to be not supported!")
                    REQUIRE(x == 0);
                }

                if (!x) 
                {
                    REQUIRE(x == 1);
                }
                REQUIRE(x == 1);
            }
        }
    }
}

TEST_CASE( "R200 streams HD Raw10", "[live] [r200] [one-camera]" )
{
    test_ds_device_streaming({{RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_RAW10, 30}});
}

//TEST_CASE("R200 Testing RGB Exposure values", "[live] [DS-device] [one-camera]")
//{
//    // The logarithmic range is [-13:1:-4]
//    test_ds_device_option(RS_OPTION_COLOR_EXPOSURE, { -13, -9, -4, -6, -10 }, {}, BEFORE_START_DEVICE | AFTER_START_DEVICE);
//}

#endif /* !defined(MAKEFILE) || ( defined(LIVE_TEST) && defined(R200_TEST) ) */