// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "catch/catch.hpp"

#include <map>
#include <memory>
#include <string>

#include "unit-tests-common.h"
#include "unit-tests-groundtruth.h"
#include "../include/librealsense2/hpp/rs_context.hpp"
//
//using namespace librealsense;
//using namespace ivcam2;
//
//const PID_t PID = L500_PID;
//
//TEST_CASE("Verify metadata", "[l500]")
//{
//    //const auto&& l500_dev = create_l500_device();
//    //auto&& l500_depth_sen = l500_dev->get_depth_sensor();
//
//    //auto&& supported_md = l500_depth_sen.meta
//    //// get metadata tags from ground truth
//
//    //// compare
//}


// TODO - Ariel - L500 - Default profiles
TEST_CASE("L500 Multistream capabilities", "[l500][device_specific]")
{
    rs2::context ctx;
    //Stream hid + color + z16 + y8 + confidence
    REQUIRE(make_context(SECTION_FROM_TEST_NAME, &ctx));

    auto&& devices = ctx.query_devices();
    auto&& l500_device = get_device_by("L500", devices);
    const auto&& confidence_prof = get_profile_by()
}
