// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////
// This set of tests is valid only for the R200 camera //
/////////////////////////////////////////////////////////

#if !defined(MAKEFILE) || ( defined(LIVE_TEST) && ( defined(LR200_TEST) || defined(ZR300_TEST) ) )

#include "unit-tests-live-ds-common.h"

#include <climits>
#include <sstream>

TEST_CASE("Streams COLOR HD 1920X1080  Raw16 30FPS", "[live] [lr200] [one-camera]")
{
    // Require only one device to be plugged in
    safe_context ctx;
    const int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(std::any_of(ds_names.begin(), ds_names.end(), [&](std::string const& s) {return s == rs_get_device_name(dev, require_no_error()); }));

    test_ds_device_streaming({ {RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_RAW16, 30} });
}

//TEST_CASE("LR200 Testing RGB Exposure values", "[live] [DS-device] [one-camera]")
//{
//    // the range is [-8:1:-4]
//    test_ds_device_option(RS_OPTION_COLOR_EXPOSURE, { -8, -7, -6, -5, -4 }, {}, BEFORE_START_DEVICE | AFTER_START_DEVICE);
//}

#endif /* !defined(MAKEFILE) || ( defined(LIVE_TEST) && ( defined(LR200_TEST) || defined(ZR300_TEST) ) ) */