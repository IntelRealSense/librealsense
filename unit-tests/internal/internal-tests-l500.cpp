// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "catch/catch.hpp"

#include <cmath>
#include <iostream>
#include <chrono>
#include <ctime>
#include <vector>
#include <algorithm>
#include <memory>

#include "../unit-tests-common.h"
#include "../unit-tests-expected.h"
#include <librealsense2/rs.hpp>
//#include <librealsense2/hpp/rs_sensor.hpp>

#include "../src/sensor.h"
#include "../src/l500/l500-factory.h"
#include "../src/l500/l500-depth.h"
#include "../src/l500/l500-motion.h"
#include "../src/l500/l500-color.h"

// TODO - Discuss with Sergey and find a better way to include this without including rs.cpp
struct rs2_context
{
    ~rs2_context() { ctx->stop(); }
    std::shared_ptr<librealsense::context> ctx;
};

std::string get_device_pid(const librealsense::device& device)
{
    return device.get_info(RS2_CAMERA_INFO_PRODUCT_ID);
}

//std::shared_ptr<librealsense::l500_device> create_l500_device()
//{
//
//}

using namespace librealsense;
using namespace librealsense::platform;

std::vector<std::shared_ptr<stream_profile_interface>> retrieve_profiles(const librealsense::synthetic_sensor& sensor, const std::vector<librealsense::stream_profile>& profiles)
{
    std::vector<std::shared_ptr<stream_profile_interface>> res;
    auto&& supported_profiles = sensor.get_stream_profiles();
    for (auto&& p : profiles)
    {
        auto profiles_are_equal = [&p](const std::shared_ptr<stream_profile_interface>& sp) { return to_profile(sp.get()) == p; };
        auto profile_it = std::find_if(supported_profiles.begin(), supported_profiles.end(), profiles_are_equal);
        res.push_back(*profile_it);
    }
    return res;
}

TEST_CASE("Sensor Resolver Test", "[L500][live][device_specific]")
{
    // check if the expected resolved profiles are equal to the resolver result

    rs2::context ctx;
    REQUIRE(make_context(SECTION_FROM_TEST_NAME, &ctx));
    auto ctxptr = std::shared_ptr<rs2_context>(ctx);
    auto&& lrsctx = ctxptr->ctx;
    auto&& be = lrsctx->get_backend();

    platform::backend_device_group devices(be.query_uvc_devices(), be.query_usb_devices(), be.query_hid_devices());
    auto info = l500_info::pick_l500_devices(lrsctx, devices);
    auto l500_info = std::dynamic_pointer_cast<librealsense::l500_info>(info[0]);
    auto device = l500_info->create(lrsctx, false);
    auto l500_color_dev = std::dynamic_pointer_cast<l500_color>(device);

    auto&& l500_sen = l500_color_dev->get_color_sensor();
    //auto l500_color_sen = std::dynamic_cast<librealsense::l500_color_sensor*>(&l500_sen);
    auto pid = get_device_pid(*l500_color_dev);
    auto expected_resolver_set = generate_sensor_resolver_profiles(pid);
    auto&& expected_resolver_pairs = expected_resolver_set[sensor_type::color_sensor];

    // iterate over all of the sensor expected resolver pairs
    for (auto&& expctd_rslvr_pr : expected_resolver_pairs)
    {
        // check if the expected resolved profiles are equal to the resolver result

        auto&& to_resolve = retrieve_profiles(l500_sen, expctd_rslvr_pr.first);
        auto&& expected_resolved_profiles = retrieve_profiles(l500_sen, expctd_rslvr_pr.second);
        auto&& resolved_profiles = l500_sen.resolve_requests(to_resolve);

        for (auto&& expctd : expected_resolved_profiles)
        {
            auto profiles_equality_predicate = [&expctd](const std::shared_ptr<stream_profile_interface>& sp)
            {
                bool res = sp->get_format() == expctd->get_format() &&
                    sp->get_stream_index() == expctd->get_stream_index() &&
                    sp->get_stream_type() == expctd->get_stream_type();
                auto vsp = As<video_stream_profile, stream_profile_interface>(sp);
                auto expct_vsp = As<video_stream_profile, stream_profile_interface>(expctd);
                if (vsp)
                    res = res && vsp->get_width() == expct_vsp->get_width() &&
                    vsp->get_height() == expct_vsp->get_height();
                return res;
            };
            expected_resolved_profiles.erase(std::remove_if(expected_resolved_profiles.begin(), expected_resolved_profiles.end(), profiles_equality_predicate));
        }

        REQUIRE(expected_resolved_profiles.empty());
    }
}

// Check processing blocks factories
// check start callbacks
// check init stream profiles
