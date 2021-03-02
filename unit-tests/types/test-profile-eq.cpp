// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../func/func-common.h"
#include "../../include/librealsense2/hpp/rs_frame.hpp"
#include <iostream>

using namespace rs2;

TEST_CASE("test video_stream_profile operator==", "[live]")
{
    auto devices = find_devices_by_product_line_or_exit(RS2_PRODUCT_LINE_ANY_INTEL);
    auto dev = devices[0];

    auto depth_sens = dev.first< rs2::depth_sensor >();

    REQUIRE_NOTHROW(depth_sens.set_option(RS2_OPTION_EMITTER_ON_OFF, 1));

    stream_profile profile0, profile1;
    std::vector< stream_profile > stream_profiles;
    REQUIRE_NOTHROW(stream_profiles = depth_sens.get_stream_profiles());
    profile0 = stream_profiles[0];
    video_stream_profile vprofile0 = profile0.as<video_stream_profile>();

    for (auto profile : stream_profiles)
    {
        if (profile == profile0)
        {
            video_stream_profile vprofile = profile.as<video_stream_profile>();
            if (vprofile0.width() != vprofile.width() ||
                vprofile0.height() != vprofile.height())
            {
                //std::cout << "compare: " << vprofile0.stream_type() << "(" << vprofile0.stream_index() << "):" << vprofile0.width() << "x" << vprofile0.height() << " with:" <<
                //    vprofile.stream_type() << "(" << vprofile.stream_index() << "):" << vprofile.width() << "x" << vprofile.height() << std::endl;
                REQUIRE(!(vprofile0 == vprofile));
            }
        }
    }
}