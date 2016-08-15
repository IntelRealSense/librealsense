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
            std::vector<rs_option> supported_options{
                RS_OPTION_COLOR_BACKLIGHT_COMPENSATION,
                RS_OPTION_COLOR_BRIGHTNESS,
                RS_OPTION_COLOR_CONTRAST,
                RS_OPTION_COLOR_EXPOSURE,
                RS_OPTION_COLOR_GAIN,
                RS_OPTION_COLOR_GAMMA,
                RS_OPTION_COLOR_HUE,
                RS_OPTION_COLOR_SATURATION,
                RS_OPTION_COLOR_SHARPNESS,
                RS_OPTION_COLOR_WHITE_BALANCE,
                RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE,
                RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE,
                RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED,
                RS_OPTION_R200_LR_GAIN,
                RS_OPTION_R200_LR_EXPOSURE,
                RS_OPTION_R200_EMITTER_ENABLED,
                RS_OPTION_R200_DEPTH_UNITS,
                RS_OPTION_R200_DEPTH_CLAMP_MIN,
                RS_OPTION_R200_DEPTH_CLAMP_MAX,
                RS_OPTION_R200_DISPARITY_MULTIPLIER,
                RS_OPTION_R200_DISPARITY_SHIFT,
                RS_OPTION_R200_AUTO_EXPOSURE_MEAN_INTENSITY_SET_POINT,
                RS_OPTION_R200_AUTO_EXPOSURE_BRIGHT_RATIO_SET_POINT,
                RS_OPTION_R200_AUTO_EXPOSURE_KP_GAIN,
                RS_OPTION_R200_AUTO_EXPOSURE_KP_EXPOSURE,
                RS_OPTION_R200_AUTO_EXPOSURE_KP_DARK_THRESHOLD,
                RS_OPTION_R200_AUTO_EXPOSURE_TOP_EDGE,
                RS_OPTION_R200_AUTO_EXPOSURE_BOTTOM_EDGE,
                RS_OPTION_R200_AUTO_EXPOSURE_LEFT_EDGE,
                RS_OPTION_R200_AUTO_EXPOSURE_RIGHT_EDGE,
                RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_DECREMENT,
                RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_INCREMENT,
                RS_OPTION_R200_DEPTH_CONTROL_MEDIAN_THRESHOLD,
                RS_OPTION_R200_DEPTH_CONTROL_SCORE_MINIMUM_THRESHOLD,
                RS_OPTION_R200_DEPTH_CONTROL_SCORE_MAXIMUM_THRESHOLD,
                RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_COUNT_THRESHOLD,
                RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_DIFFERENCE_THRESHOLD,
                RS_OPTION_R200_DEPTH_CONTROL_SECOND_PEAK_THRESHOLD,
                RS_OPTION_R200_DEPTH_CONTROL_NEIGHBOR_THRESHOLD,
                RS_OPTION_R200_DEPTH_CONTROL_LR_THRESHOLD,
                RS_OPTION_FRAMES_QUEUE_SIZE
            };

            for (int i = 0; i<RS_OPTION_COUNT; ++i)
            {
                if (std::find(std::begin(supported_options), std::end(supported_options), i) != std::end(supported_options))
                {
                    REQUIRE(rs_device_supports_option(dev, (rs_option)i, require_no_error()) == 1);
                }
                else
                {
                    REQUIRE(rs_device_supports_option(dev, (rs_option)i, require_no_error()) == 0);
                }
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