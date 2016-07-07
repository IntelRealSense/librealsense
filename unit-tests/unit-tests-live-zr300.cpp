// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////
// This set of tests is valid only for the R200 camera //
/////////////////////////////////////////////////////////

#include "catch/catch.hpp"

#include "librealsense/rs.hpp"
#include "unit-tests-live-ds-common.h"

#include <climits>
#include <sstream>

static uint32_t timestamp_frame_counter = 0;
static std::vector<rs::motion_data> accel_frames = {};
static std::vector<rs::motion_data> gyro_frames = {};

auto motion_handler = [](rs::motion_data entry)
{
    // DEBUG SESSION

    // Collect data
    if (rs_event_source::RS_EVENT_IMU_ACCEL == entry.timestamp_data.source_id)
    {
        accel_frames.push_back(entry);
    }
    if (rs_event_source::RS_EVENT_IMU_GYRO == entry.timestamp_data.source_id)
    {
        gyro_frames.push_back(entry);
    }
};

// ... and the timestamp packets (DS4.1/FishEye Frame, GPIOS...)
auto timestamp_handler = [](rs::timestamp_data entry)
{
    std::cout << "Timestamp arrived, timestamp: " << entry.timestamp << std::endl;
};

TEST_CASE("ZR300 devices support all required options", "[live] [DS-device]")
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

        SECTION("ZR300 supports DS-Line standard UVC controls and ZR300 Motion Module controls, and nothing else")
        {
            const int supported_options[] = {
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
                RS_OPTION_ZR300_GYRO_BANDWIDTH,
                RS_OPTION_ZR300_GYRO_RANGE,
                RS_OPTION_ZR300_ACCELEROMETER_BANDWIDTH,
                RS_OPTION_ZR300_ACCELEROMETER_RANGE,
                RS_OPTION_ZR300_MOTION_MODULE_TIME_SEED,
                RS_OPTION_ZR300_MOTION_MODULE_ACTIVE,
                RS_OPTION_FISHEYE_COLOR_EXPOSURE,
                RS_OPTION_FISHEYE_COLOR_GAIN,
                RS_OPTION_FISHEYE_STROBE,
                RS_OPTION_FISHEYE_EXT_TRIG
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

TEST_CASE("ZR300 Motion Module Data Streaming", "[live] [DS-device]")
{
    // Require only one device to be plugged in
    safe_context ctx;
    const int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count == 1);

    // For each device
    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);

    const char * name = rs_get_device_name(dev, require_no_error());
    REQUIRE(name == std::string("Intel RealSense ZR300"));

    REQUIRE(rs_supports(dev, rs_capabilities::RS_CAPABILITIES_MOTION_EVENTS, require_no_error()));

    for (int ii = 0; ii < 20; ii++)
    {
		std::cout << "Iteration num " << ii + 1 << std::endl;
        rs_enable_motion_tracking_cpp(dev, new rs::motion_callback(motion_handler), new rs::timestamp_callback(timestamp_handler), require_no_error());

        timestamp_frame_counter = 0;
        accel_frames.clear();
        gyro_frames.clear();

        // 3. Start generating motion-tracking data
        rs_start_source(dev, rs_source::RS_SOURCE_MOTION_TRACKING, require_no_error());

        // Collect data for a while
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // 4. stop data acquisition
        rs_stop_source(dev, rs_source::RS_SOURCE_MOTION_TRACKING, require_no_error());

        // Validate acquired data
        REQUIRE(accel_frames.size()>0);
        REQUIRE(gyro_frames.size()>0);
        //REQUIRE(std::any_of(ds_names.begin(), ds_names.end(), [&](std::string const& s) {return s == rs_get_device_name(dev, require_no_error()); }));
        REQUIRE(std::all_of(accel_frames.begin(), accel_frames.end(), [](rs::motion_data const& entry) { return entry.timestamp_data.source_id == rs_event_source::RS_EVENT_IMU_ACCEL; }));
        REQUIRE(std::all_of(gyro_frames.begin(), gyro_frames.end(), [](rs::motion_data const& entry) { return entry.timestamp_data.source_id == rs_event_source::RS_EVENT_IMU_GYRO; }));
        for (size_t i = 0; i < (gyro_frames.size() - 1); i++)
        {
            REQUIRE(gyro_frames[i].timestamp_data.frame_number < gyro_frames[i + 1].timestamp_data.frame_number);               
        }

        for (size_t i = 0; i < (accel_frames.size() - 1); i++)
        {
            REQUIRE(accel_frames[i].timestamp_data.frame_number < accel_frames[i + 1].timestamp_data.frame_number);
        }

        // 5. reset previous settings formotion data handlers
        rs_disable_motion_tracking(dev, require_no_error());
    }
}

