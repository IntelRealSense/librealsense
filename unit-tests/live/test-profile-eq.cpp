// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#test:device L500*

#include "live-common.h"
#include <iostream>

using namespace rs2;

TEST_CASE("test video_stream_profile operator==", "[live]")
{
    // Test that for 2 video_stream_profile objects, if width and height are different
    // then, video_stream_profile.operator== returns false.

    auto devices = find_devices_by_product_line_or_exit(RS2_PRODUCT_LINE_DEPTH);
    auto dev = devices[0];

    auto depth_sens = dev.first< rs2::depth_sensor >();

    stream_profile profile0, profile1;
    std::vector< stream_profile > stream_profiles;
    REQUIRE_NOTHROW(stream_profiles = depth_sens.get_stream_profiles());
    for (auto profile : stream_profiles)
    {
        if (profile.is<video_stream_profile>())
        {
            profile0 = profile;
        }
    }
    if (!profile0) return;
    video_stream_profile vprofile0 = profile0.as<video_stream_profile>();

    for (auto profile : stream_profiles)
    {
        if (!profile.is<video_stream_profile>()) continue;
        if (profile == profile0)
        {
            video_stream_profile vprofile = profile.as<video_stream_profile>();
            if (vprofile0.width() == vprofile.width() &&
                vprofile0.height() == vprofile.height())
            {
                REQUIRE(vprofile0 == vprofile);
            }
            else
            {
                REQUIRE(!(vprofile0 == vprofile));
            }
        }
    }
}
