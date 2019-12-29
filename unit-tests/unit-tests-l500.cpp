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

inline rs2::device get_l500_device()
{
    return get_device_by(L500_TAG, get_all_devices());
}

inline rs2::config create_l500_config()
{
    rs2::config cfg;
    cfg.enable_device(get_device_serial(get_l500_device()));
    return cfg;
}

// TODO - Ariel - pipeline resources deallocation

// TODO - Ariel - Extrinsics test

// TODO - Ariel - stream fps > 25?

// TODO - Ariel - Configure all supported profiles and test frame arrived.


// TODO - Ariel - Consistency between frame profile and requested profile

// TODO - Ariel - IR and Depth resolution must be equal

// TODO - Ariel - gyro and accelerometer data not stuck?

// TODO - Ariel - profiles consistency with configuration and frame result

// TODO - Ariel - recordings - start stop

// TODO - Ariel - recordings - via pipeline

// TODO - Ariel - playback - 

// TODO - Ariel - resources deallocation - stay as before.

TEST_CASE("L500 Multistream", "[l500][device_specific]")
{
    // Check all of the device stream profiles are playing together
    // Stream hid + color + z16 + y8 + confidence

    rs2::context ctx;
    REQUIRE(make_context(SECTION_FROM_TEST_NAME, &ctx));

    const int width = 640, height = 480, fps = 30, gyro_fps = 100, accel_fps = 50, idx = 0;

    auto l500_device = get_l500_device();
    auto&& l500_sensors = l500_device.query_sensors();

    auto depth_snsr =   get_sensor_by(l500_sensors, sensor_type::depth_sensor);
    auto color_snsr =   get_sensor_by(l500_sensors, sensor_type::color_sensor);
    auto motion_snsr =  get_sensor_by(l500_sensors, sensor_type::motion_sensor);

    std::vector<rs2::stream_profile> depth_profiles;
    std::vector<rs2::stream_profile> color_profiles;
    std::vector<rs2::stream_profile> motion_profiles;

    depth_profiles.push_back(get_profile_by(depth_snsr, { RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE, idx, width, height, fps }));
    depth_profiles.push_back(get_profile_by(depth_snsr, { RS2_FORMAT_Z16, RS2_STREAM_DEPTH, idx, width, height, fps }));
    depth_profiles.push_back(get_profile_by(depth_snsr, { RS2_FORMAT_Y8, RS2_STREAM_INFRARED, idx, width, height, fps }));

    color_profiles.push_back(get_profile_by(color_snsr, { RS2_FORMAT_RGB8, RS2_STREAM_COLOR, idx, width, height, fps }));

    motion_profiles.push_back(get_profile_by(motion_snsr, { RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_ACCEL, idx, 0, 0, accel_fps }));
    motion_profiles.push_back(get_profile_by(motion_snsr, { RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO, idx, 0, 0, gyro_fps }));

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

TEST_CASE("L500 Default Profiles", "[l500][device_specific]")
{
    // Check all expected default profiles are requested by the pipeline.

    rs2::context ctx;
    REQUIRE(make_context(SECTION_FROM_TEST_NAME, &ctx));

    auto l500_device = get_l500_device();
    rs2::config cfg = create_l500_config();

    // Start a default pipeline with l500 device
    rs2::pipeline pipe(ctx);

    auto pipe_prof = pipe.start(cfg);
    auto&& default_profiles = pipe_prof.get_streams();

    // Check default profiles configured are as expected
    const auto&& pid = l500_device.get_info(RS2_CAMERA_INFO_PRODUCT_ID);
    auto&& expected_default_profiles = generate_device_default_profiles(pid);

    REQUIRE(expected_default_profiles.size() == default_profiles.size());

    for (auto&& dp : default_profiles)
    {
        expected_default_profiles.erase(librealsense::to_profile(dp.get()->profile));
    }

    REQUIRE(expected_default_profiles.empty());
}

TEST_CASE("L500 Verify Sensors", "[l500][device_specific]")
{
    // Verify l500 device has hid, depth and color sensors.

    rs2::context ctx;
    REQUIRE(make_context(SECTION_FROM_TEST_NAME, &ctx));

    auto l500_device = get_l500_device();
    auto&& l500_sensors = l500_device.query_sensors();

    REQUIRE_NOTHROW(auto cs = get_sensor_by(l500_sensors, sensor_type::color_sensor));
    REQUIRE_NOTHROW(auto ds = get_sensor_by(l500_sensors, sensor_type::depth_sensor));
    REQUIRE_NOTHROW(auto ms = get_sensor_by(l500_sensors, sensor_type::motion_sensor));
}


TEST_CASE("L500 zero order sanity", "[live][l500][device_specific]") {
    //Require at least one device to be plugged in
    rs2::context ctx;
    const int RETRIES = 30;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<rs2::sensor> sensors;
        REQUIRE_NOTHROW(sensors = ctx.query_all_sensors());
        REQUIRE(sensors.size() > 0);

        for (auto sensor : sensors)
        {
            auto processing_blocks = sensor.get_recommended_filters();
            auto zo = std::find_if(processing_blocks.begin(), processing_blocks.end(), [](rs2::filter f)
                {
                    return f.is<rs2::zero_order_invalidation>();
                });

            if (zo != processing_blocks.end())
            {
                rs2::config c;
                c.enable_stream(RS2_STREAM_DEPTH);
                c.enable_stream(RS2_STREAM_INFRARED);
                c.enable_stream(RS2_STREAM_CONFIDENCE);

                rs2::pipeline p;
                p.start(c);
                rs2::frame frame;

                std::map<rs2_stream, bool> stream_arrived;
                stream_arrived[RS2_STREAM_DEPTH] = false;
                stream_arrived[RS2_STREAM_INFRARED] = false;
                stream_arrived[RS2_STREAM_CONFIDENCE] = false;

                for (auto i = 0; i < RETRIES; i++)
                {
                    REQUIRE_NOTHROW(frame = p.wait_for_frames(15000));
                    auto res = zo->process(frame);
                    if (res.is<rs2::frameset>())
                    {
                        auto set = res.as<rs2::frameset>();
                        REQUIRE(set.size() == stream_arrived.size());   // depth, ir, confidance
                        for (auto&& f : set)
                        {
                            stream_arrived[f.get_profile().stream_type()] = true;
                        }
                        auto stream_missing = std::find_if(stream_arrived.begin(), stream_arrived.end(), [](std::pair< rs2_stream, bool> item)
                            {
                                return !item.second;
                            });

                        REQUIRE(stream_missing == stream_arrived.end());
                    }
                }
            }
        }
    }
}

// TODO - Ariel - long - short ranges tests

// TODO - Ariel - rotated profiles check?
