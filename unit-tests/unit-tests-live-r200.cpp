// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////
// This set of tests is valid only for the R200 camera //
/////////////////////////////////////////////////////////


#include "catch/catch.hpp"

#include "unit-tests-live-ds-common.h"

#include <climits>
#include <sstream>

/////////////////////
// Streaming tests //
/////////////////////


TEST_CASE( "R200 streams HD Raw10", "[live] [r200] [one-camera]" )
{
    test_ds_device_streaming({{RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_RAW10, 30}});
}

TEST_CASE("R200 Testing RGB Exposure values", "[live] [DS-device] [one-camera]")
{
    // The formula is 2^(exposure [-8..-4]) / 10000
    SECTION("Testing without Streaming")
    {
        test_ds_device_option(RS_OPTION_COLOR_EXPOSURE, { 39, 78, 156, 313, 625 }, {}, BEFORE_START_DEVICE);
    }

    SECTION("Testing while Streaming")
    {
        test_ds_device_option(RS_OPTION_COLOR_EXPOSURE, { 39, 78, 156, 313, 625 }, {}, AFTER_START_DEVICE);
    }
}
