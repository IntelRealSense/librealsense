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

TEST_CASE("R200 device-unique options support", "[live] [DS-device]")
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
        REQUIRE(ds_names[Intel_R200] == rs_get_device_name(dev, require_no_error()));

        // Prerequisite for the following options list
        rs_set_device_option(dev, RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED, 1.0, require_no_error());

        const int supported_options[] = {
            RS_OPTION_R200_AUTO_EXPOSURE_BRIGHT_RATIO_SET_POINT,
            RS_OPTION_R200_AUTO_EXPOSURE_KP_GAIN,
            RS_OPTION_R200_AUTO_EXPOSURE_KP_EXPOSURE,
            RS_OPTION_R200_AUTO_EXPOSURE_KP_DARK_THRESHOLD,
        };

        std::stringstream ss;
        for (auto opt : supported_options)
        {
            ss.str(""); ss << "Verifying support for R200-specific: " << rs_option_to_string((rs_option)i);
            REQUIRE(rs_device_supports_option(dev, (rs_option)i, require_no_error()) == 1);
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
