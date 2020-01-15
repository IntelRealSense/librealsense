// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "catch/catch.hpp"

#include <map>
#include <memory>
#include <string>

#include "unit-tests-common.h"
#include "unit-tests-expected.h"
#include "../include/librealsense2/hpp/rs_context.hpp"
#include "librealsense2/hpp/rs_types.hpp"
#include "librealsense2/hpp/rs_frame.hpp"

inline rs2::device get_l500_device(const rs2::context& ctx)
{
    return get_device_by(L500_TAG, ctx.query_devices());
}

inline rs2::config create_l500_config(const rs2::context& ctx)
{
    rs2::config cfg;
    cfg.enable_device(get_serial(get_l500_device(ctx)));
    return cfg;
}

TEST_CASE("L500 - Multistream", "[L500][device_specific]")
{
    // Check all of the device stream profiles are playing together
    // Stream hid + color + z16 + y8 + confidence

    rs2::context ctx;
    REQUIRE(make_context(SECTION_FROM_TEST_NAME, &ctx));

    const int width = 640, height = 480, fps = 30, motion_fps = 100, idx = 0;

    auto l500_device = get_l500_device(ctx);
    auto&& l500_sensors = l500_device.query_sensors();

    rs2::sensor depth_snsr;
    rs2::sensor color_snsr;
    rs2::sensor motion_snsr;

    for (auto&& s : l500_sensors)
    {
        if (s.is<rs2::depth_sensor>())
            depth_snsr = s;
        else if (s.is<rs2::motion_sensor>())
            motion_snsr = s;
        else if (s.is<rs2::color_sensor>())
            color_snsr = s;
    }

    std::vector<rs2::stream_profile> depth_profiles;
    std::vector<rs2::stream_profile> color_profiles;
    std::vector<rs2::stream_profile> motion_profiles;

    depth_profiles.push_back(get_profile_by(depth_snsr, { RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE, idx, width, height, fps }));
    depth_profiles.push_back(get_profile_by(depth_snsr, { RS2_FORMAT_Z16, RS2_STREAM_DEPTH, idx, width, height, fps }));
    depth_profiles.push_back(get_profile_by(depth_snsr, { RS2_FORMAT_Y8, RS2_STREAM_INFRARED, idx, width, height, fps }));

    color_profiles.push_back(get_profile_by(color_snsr, { RS2_FORMAT_RGB8, RS2_STREAM_COLOR, idx, width, height, fps }));

    motion_profiles.push_back(get_profile_by(motion_snsr, { RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ACCEL, idx, 0, 0, motion_fps }));
    motion_profiles.push_back(get_profile_by(motion_snsr, { RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO, idx, 0, 0, motion_fps }));

    REQUIRE_NOTHROW(depth_snsr.open(depth_profiles));
    REQUIRE_NOTHROW(color_snsr.open(color_profiles));
    REQUIRE_NOTHROW(motion_snsr.open(motion_profiles));

    REQUIRE_NOTHROW(depth_snsr.start([](rs2::frame f) {}));
    REQUIRE_NOTHROW(color_snsr.start([](rs2::frame f) {}));
    REQUIRE_NOTHROW(motion_snsr.start([](rs2::frame f) {}));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    REQUIRE_NOTHROW(depth_snsr.stop());
    REQUIRE_NOTHROW(color_snsr.stop());
    REQUIRE_NOTHROW(motion_snsr.stop());
}

TEST_CASE("L500 - Pipeline - single frame per request", "[live][L500][device_specific][pipeline]")
{
    // Config IR + Z16 + Confidence, Check that each requested profile yields one frame.
    int iterations = 5;

    // Create a pipeline
    rs2::context ctx;
    REQUIRE(make_context(SECTION_FROM_TEST_NAME, &ctx));

    rs2::config cfg;
    rs2::pipeline pipe(ctx);

    std::vector<rs2_stream> streams {RS2_STREAM_DEPTH, RS2_STREAM_INFRARED, };
    std::unordered_map<rs2_stream, int> stream_histogram;

    // Enable IR, Z16, Confidence streams
    for (auto&& stream : streams)
        cfg.enable_stream(stream);

    pipe.start(cfg);

    for (size_t i = 0; i < iterations; i++)
    {
        auto fs = pipe.wait_for_frames();
        for (auto&& f : fs)
        {
            stream_histogram[f.get_profile().stream_type()]++;
        }
    }

    pipe.stop();

    // Verify only one frame per stream type
    std::string duplicated_stream;
    auto bigger_than_one_predicate = [iterations, &duplicated_stream](const std::pair<rs2_stream, int>& hist) { 
        if (hist.second > iterations)
        {
            duplicated_stream = rs2_stream_to_string(hist.first);
            return true;
        }
        return false;
    };
    std::string error_msg = "A duplicated frame arrived from stream";
    CAPTURE(error_msg, duplicated_stream);
    REQUIRE(std::none_of(stream_histogram.begin(), stream_histogram.end(), bigger_than_one_predicate));
}

TEST_CASE("L500 - Verify Sensors", "[L500][device_specific]")
{
    // Verify l500 device has hid, depth and color sensors.

    rs2::context ctx;
    REQUIRE(make_context(SECTION_FROM_TEST_NAME, &ctx));

    auto l500_device = get_l500_device(ctx);
    auto&& l500_sensors = l500_device.query_sensors();

    bool has_color_sensor = false;
    bool has_depth_sensor = false;
    bool has_motion_sensor = false;

    for (auto&& s : l500_sensors)
    {
        if (s.is<rs2::depth_sensor>())
            has_depth_sensor = true;
        else if (s.is<rs2::motion_sensor>())
            has_motion_sensor = true;
        else if (s.is<rs2::color_sensor>())
            has_color_sensor = true;
    }

    std::string error_msg = "L500 device is missing a sensor";
    REQUIRE(has_color_sensor + has_depth_sensor + has_motion_sensor == 3);
}

template<class T>
void require_first_contains_second(std::vector<T> first, std::vector<T> second, std::string error_msg)
{
    for (auto&& f : first)
    {
        second.erase(std::remove(second.begin(), second.end(), f), second.end());
    }
    CAPTURE(error_msg);
    std::stringstream not_contained;
    for (auto&& s : second)
        not_contained << s << std::endl;
    CAPTURE(not_contained.str());
    REQUIRE(second.empty());
}

// TODO - Following tests should move into live later and be recorded.
// TODO - Remove [L500][device_specific] tags from them after moved.
// TODO - When expanding to other devices, add usb type check...
TEST_CASE("Verify Default Profiles", "[L500][device_specific]")
{
    // Check all expected default profiles are requested by the pipeline.

    rs2::context ctx;
    REQUIRE(make_context(SECTION_FROM_TEST_NAME, &ctx));

    auto l500_device = get_l500_device(ctx);
    rs2::config cfg = create_l500_config(ctx);

    // Start a default pipeline with l500 device
    rs2::pipeline pipe(ctx);

    auto pipe_prof = pipe.start(cfg);
    auto&& default_profiles = to_profiles(pipe_prof.get_streams());
    pipe.stop();

    // Check default profiles configured are as expected
    const auto&& pid = l500_device.get_info(RS2_CAMERA_INFO_PRODUCT_ID);
    auto&& expected_default_profiles = generate_device_default_profiles(pid);

    // Verify that expected default profiles contains the actual retrieved default profiles
    require_first_contains_second(expected_default_profiles, default_profiles, "There were default profiles which didn't find a match with expected");

    // Verify that actual retrieved default profiles contains the expected default profiles 
    require_first_contains_second(default_profiles, expected_default_profiles, "There were expected profiles which didn't find a match with retrieved default profiles");
}

TEST_CASE("Verify device profiles", "[live][L500][device_specific]")
{
    // Verify all expected profiles are supported by the devices
    rs2::context ctx;
    REQUIRE(make_context(SECTION_FROM_TEST_NAME, &ctx));
    const auto&& devices = ctx.query_devices();

    for (auto&& d : devices)
    {
        auto&& expected_profiles = generate_device_profiles(d);
        std::vector<rs2::stream_profile> supported_profiles;

        // For each device's sensor
        for (auto&& s : d.query_sensors())
        {
            // Get supported profiles
            auto&& sp = s.get_stream_profiles();
            supported_profiles.insert(supported_profiles.end(), sp.begin(), sp.end());
        }

        std::vector<librealsense::stream_profile> lrs_supported_profiles = to_profiles(supported_profiles);

        // Verify that expected profiles contains the actual retrieved profiles
        require_first_contains_second(expected_profiles, lrs_supported_profiles, "There were actual retrieved profiles which didn't find a match with expected profiles");

        // Verify that actual retrieved profiles contains the expected profiles 
        require_first_contains_second(lrs_supported_profiles, expected_profiles, "There were expected profiles which didn't find a match with actual retrieved profiles");
    }
}

TEST_CASE("Verify device options", "[live][L500][device_specific]")
{
    // Verify all expected options are supported by the device

    rs2::context ctx;
    REQUIRE(make_context(SECTION_FROM_TEST_NAME, &ctx));
    const auto&& devices = ctx.query_devices();

    for (auto&& d : devices)
    {
        const auto pid = d.get_info(RS2_CAMERA_INFO_PRODUCT_ID);
        auto&& gt_options = generate_device_options(pid);
        std::unordered_set<int> supported_options;

        // For each device's sensor
        for (auto&& s : d.query_sensors())
        {
            // Get supported options
            auto&& so = s.get_supported_options();
            supported_options.insert(so.begin(), so.end());
        }

        // Verify that expected options contains the actual retrieved options
        require_first_contains_second(gt_options, { supported_options.begin(), supported_options.end() }, "There were actual retrieved options which didn't find a match with expected options");

        // Verify that actual retrieved options contains the expected options 
        require_first_contains_second({ supported_options.begin(), supported_options.end() }, gt_options, "There were expected options which didn't find a match with actual retrieved options");
    }
}

TEST_CASE("Stream All Profiles", "[live][heavy_test][L500][device_specific]")
{
    // Iterate over all of the profiles and stream them.
    rs2::context ctx;
    REQUIRE(make_context(SECTION_FROM_TEST_NAME, &ctx));
    const auto&& devices = ctx.query_devices();

    // Stream everything
    for (auto&& d : devices)
    {
        auto profiles = generate_device_profiles(d);
        for (auto&& profile : profiles)
        {
            rs2::pipeline pipe(ctx);
            rs2::config cfg;
            std::cout << "Streaming:\n" << profile.to_string() << std::endl;
            cfg.enable_stream(profile.stream, profile.index, profile.width, profile.height, profile.format, profile.fps);
            REQUIRE_NOTHROW(pipe.start(cfg));

            for (size_t i = 0; i < 20; i++)
                REQUIRE_NOTHROW(pipe.wait_for_frames());

            REQUIRE_NOTHROW(pipe.stop());
        }
    }
}
