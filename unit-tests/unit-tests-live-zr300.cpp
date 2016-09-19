// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////
// This set of tests is valid only for the R200 camera //
/////////////////////////////////////////////////////////

#if !defined(MAKEFILE) || ( defined(LIVE_TEST) && defined(ZR300_TEST) )

#include "catch/catch.hpp"

#include "librealsense/rs.hpp"
#include "unit-tests-live-ds-common.h"

#include <climits>
#include <sstream>
#include <numeric>

static std::vector<rs::motion_data> accel_frames = {};
static std::vector<rs::motion_data> gyro_frames = {};
static std::vector<rs::timestamp_data> fisheye_timestamp_events = {};
static std::vector<rs::timestamp_data> depth_timestamp_events = {};

auto motion_handler = [](rs::motion_data entry)
{
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
    if (rs_event_source::RS_EVENT_IMU_MOTION_CAM == entry.source_id)
    {
        fisheye_timestamp_events.push_back(entry);
    }
    if (rs_event_source::RS_EVENT_IMU_DEPTH_CAM == entry.source_id)
    {
        depth_timestamp_events.push_back(entry);
    }
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
        REQUIRE(ds_names[Intel_ZR300] == rs_get_device_name(dev, require_no_error()));

        rs_set_device_option(dev, RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED, 1.0, require_no_error());

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
                RS_OPTION_R200_AUTO_EXPOSURE_MEAN_INTENSITY_SET_POINT,
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
                RS_OPTION_FISHEYE_EXPOSURE,
                RS_OPTION_FISHEYE_GAIN,
                RS_OPTION_FISHEYE_STROBE,
                RS_OPTION_FISHEYE_EXTERNAL_TRIGGER,
                RS_OPTION_FISHEYE_ENABLE_AUTO_EXPOSURE,
                RS_OPTION_FISHEYE_AUTO_EXPOSURE_MODE,
                RS_OPTION_FISHEYE_AUTO_EXPOSURE_ANTIFLICKER_RATE,
                RS_OPTION_FISHEYE_AUTO_EXPOSURE_PIXEL_SAMPLE_RATE,
                RS_OPTION_FISHEYE_AUTO_EXPOSURE_SKIP_FRAMES,
                RS_OPTION_FRAMES_QUEUE_SIZE,
                RS_OPTION_HARDWARE_LOGGER_ENABLED
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

TEST_CASE("ZR300 Motion Module Data Streaming Validation", "[live] [DS-device]")
{
    // Require only one device to be plugged in
    safe_context ctx;
    const int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count == 1);

    // For each device
    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);

    REQUIRE(ds_names[Intel_ZR300] == rs_get_device_name(dev, require_no_error()));

    REQUIRE(rs_supports(dev, rs_capabilities::RS_CAPABILITIES_MOTION_EVENTS, require_no_error()));

    unsigned int active_period_ms = 5000; // Time for the application to generate and collect data
    const unsigned int gyro_bandwidth_fps = 200;
    const unsigned int accel_bandwidth_fps = 250; // Predefined rate
    const double allowed_deviation = 0.03; // The frame rates can vary within the predefined limit
    const int video_fps = 60;

    for (int ii = 0; ii < 2; ii++)
    {
        INFO("Iteration num " << ii + 1 << " has started ");

        accel_frames.clear();
        gyro_frames.clear();
        fisheye_timestamp_events.clear();
        depth_timestamp_events.clear();

        // Configure motion events callbacks
        rs_enable_motion_tracking_cpp(dev, new rs::motion_callback(motion_handler), new rs::timestamp_callback(timestamp_handler), require_no_error());

        // Configure which video streams need to active
        rs_enable_stream(dev, RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, video_fps, require_no_error());
        rs_enable_stream(dev, RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, video_fps, require_no_error());
        rs_enable_stream(dev, RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8, video_fps, require_no_error());
        rs_enable_stream(dev, RS_STREAM_INFRARED2, 640, 480, RS_FORMAT_Y8, video_fps, require_no_error());
        rs_enable_stream(dev, RS_STREAM_FISHEYE, 640, 480, RS_FORMAT_RAW8, video_fps, require_no_error());

        // Activate strobe to enforce timestamp events ( required by spec)
        rs_set_device_option(dev,rs_option::RS_OPTION_FISHEYE_STROBE, 1, require_no_error());

        // 3. Start generating motion-tracking data
        rs_start_source(dev, rs_source::RS_SOURCE_ALL, require_no_error());

        // Collect data for a while
        std::this_thread::sleep_for(std::chrono::milliseconds(active_period_ms));

        // 4. stop data acquisition
        rs_stop_source(dev, rs_source::RS_SOURCE_ALL, require_no_error());

        // 5. reset previous settings formotion data handlers
        rs_disable_motion_tracking(dev, require_no_error());

        // i. Sanity Tests
        REQUIRE(accel_frames.size() > 0);
        REQUIRE(gyro_frames.size() > 0);
        REQUIRE(fisheye_timestamp_events.size() > 0);
        REQUIRE(depth_timestamp_events.size() > 0);
        INFO(gyro_frames.size() << " Gyro packets received over " << active_period_ms*0.001 << "sec sampling");
        INFO(accel_frames.size() << " Accelerometer  packets received");
        INFO(fisheye_timestamp_events.size() << " Fisheye timestamp  packets received");
        INFO(depth_timestamp_events.size() << " Depth timestamp  packets received");

        //  Verify predefined frame rates
        REQUIRE(gyro_frames.size() / (double)active_period_ms == Approx(gyro_bandwidth_fps).epsilon(gyro_bandwidth_fps*allowed_deviation));
        REQUIRE(gyro_frames.size() / (double)active_period_ms == Approx(accel_bandwidth_fps).epsilon(accel_bandwidth_fps*allowed_deviation));

        REQUIRE(fisheye_timestamp_events.size() / (double)active_period_ms == Approx(video_fps).epsilon(video_fps*allowed_deviation));
        REQUIRE(depth_timestamp_events.size() / (double)active_period_ms == Approx(video_fps).epsilon(video_fps*allowed_deviation));

        // ii. Verify that the recorded data
        REQUIRE(std::all_of(accel_frames.begin(), accel_frames.end(), [](rs::motion_data const& entry) { return entry.timestamp_data.source_id == rs_event_source::RS_EVENT_IMU_ACCEL; }));
        REQUIRE(std::all_of(gyro_frames.begin(), gyro_frames.end(), [](rs::motion_data const& entry) { return entry.timestamp_data.source_id == rs_event_source::RS_EVENT_IMU_GYRO; }));
        REQUIRE(std::all_of(fisheye_timestamp_events.begin(), fisheye_timestamp_events.end(), [](rs::timestamp_data const& entry) { return entry.source_id == rs_event_source::RS_EVENT_IMU_MOTION_CAM; }));
        REQUIRE(std::all_of(depth_timestamp_events.begin(), depth_timestamp_events.end(), [](rs::timestamp_data const& entry) { return entry.source_id == rs_event_source::RS_EVENT_IMU_DEPTH_CAM; }));

        // iii. Validate data consistency
        for (size_t i = 0; i < (gyro_frames.size() - 1); i++)
            REQUIRE(gyro_frames[i].timestamp_data.frame_number <= gyro_frames[i + 1].timestamp_data.frame_number);

        for (size_t i = 0; i < (accel_frames.size() - 1); i++)
            REQUIRE(accel_frames[i].timestamp_data.frame_number <= accel_frames[i + 1].timestamp_data.frame_number);

        for (size_t i = 0; i < (fisheye_timestamp_events.size() - 1); i++)
            REQUIRE(fisheye_timestamp_events[i].frame_number <= fisheye_timestamp_events[i + 1].frame_number);

        for (size_t i = 0; i < (depth_timestamp_events.size() - 1); i++)
            REQUIRE(depth_timestamp_events[i].frame_number <= depth_timestamp_events[i + 1].frame_number);

        // Frame numbers statistics
        std::vector<unsigned long long> gyro_frame_numbers, accel_frame_numbers, fisheye_frame_numbers, depth_cam_frame_numbers;
        std::vector<unsigned long long> gyro_adjacent_diff, accel_adjacent_diff, fisheye_adjacent_diff, depth_adjacent_diff;

        // Extract the data
        for (size_t i = 0; i < gyro_frames.size(); i++) gyro_frame_numbers.push_back(gyro_frames[i].timestamp_data.frame_number);
        for (size_t i = 0; i < accel_frames.size(); i++) accel_frame_numbers.push_back(accel_frames[i].timestamp_data.frame_number);
        for (size_t i = 0; i < fisheye_timestamp_events.size(); i++) fisheye_frame_numbers.push_back(fisheye_timestamp_events[i].frame_number);
        for (size_t i = 0; i < depth_timestamp_events.size(); i++) depth_cam_frame_numbers.push_back(depth_timestamp_events[i].frame_number);

        gyro_adjacent_diff.resize(gyro_frame_numbers.size());
        accel_adjacent_diff.resize(accel_frame_numbers.size());
        fisheye_adjacent_diff.resize(fisheye_frame_numbers.size());
        depth_adjacent_diff.resize(depth_cam_frame_numbers.size());

        std::adjacent_difference(gyro_frame_numbers.begin(), gyro_frame_numbers.end(), gyro_adjacent_diff.begin());
        std::adjacent_difference(accel_frame_numbers.begin(), accel_frame_numbers.end(), accel_adjacent_diff.begin());
        std::adjacent_difference(fisheye_frame_numbers.begin(), fisheye_frame_numbers.end(), fisheye_adjacent_diff.begin());
        std::adjacent_difference(depth_cam_frame_numbers.begin(), depth_cam_frame_numbers.end(), depth_adjacent_diff.begin());
        // Fix the value of the first cell
        accel_adjacent_diff[0] = gyro_adjacent_diff[0] = fisheye_adjacent_diff[0] = depth_adjacent_diff[0] = 1;

        std::vector<unsigned int> gyro_frame_diff_bins, accel_frame_diff_bins, fisheye_frame_bins, depth_cam_frames_bins;
        const uint32_t max_frame_diff_allowed = 100;
        gyro_frame_diff_bins.resize(max_frame_diff_allowed);      // Calculate the distribution of the frame number gaps up to 100 frames
        accel_frame_diff_bins.resize(max_frame_diff_allowed);      // Calculate the distribution of the frame number gaps up to 100 frames
        fisheye_frame_bins.resize(max_frame_diff_allowed);      // Calculate the distribution of the frame number gaps up to 100 frames
        depth_cam_frames_bins.resize(max_frame_diff_allowed);      // Calculate the distribution of the frame number gaps up to 100 frames

        // Calculate distribution
        std::for_each(gyro_adjacent_diff.begin(), gyro_adjacent_diff.end(), [max_frame_diff_allowed, &gyro_frame_diff_bins](unsigned long n)
        { REQUIRE(n < max_frame_diff_allowed); gyro_frame_diff_bins[n]++; });
        std::for_each(accel_adjacent_diff.begin(), accel_adjacent_diff.end(), [max_frame_diff_allowed, &accel_frame_diff_bins](unsigned long n)
        { REQUIRE(n < max_frame_diff_allowed); accel_frame_diff_bins[n]++; });
        std::for_each(fisheye_adjacent_diff.begin(), fisheye_adjacent_diff.end(), [max_frame_diff_allowed, &fisheye_frame_bins](unsigned long n)
        { REQUIRE(n < max_frame_diff_allowed); fisheye_frame_bins[n]++; });
        std::for_each(depth_adjacent_diff.begin(), depth_adjacent_diff.end(), [max_frame_diff_allowed, &depth_cam_frames_bins](unsigned long n)
        { REQUIRE(n < max_frame_diff_allowed); depth_cam_frames_bins[n]++; });

        // display the distributions
        INFO(" gyro frame num differences: ");
        for (size_t i = 0; i < gyro_frame_diff_bins.size(); i++)  if (gyro_frame_diff_bins[i]) INFO("[" << i << " = " << gyro_frame_diff_bins[i] << "], ");
        INFO(" accel frame num differences: ");
        for (size_t i = 0; i < accel_frame_diff_bins.size(); i++) if (accel_frame_diff_bins[i]) INFO("[" << i << " = " << accel_frame_diff_bins[i] << "], ");
        INFO(" fisheye frame num differences: ");
        for (size_t i = 0; i < fisheye_frame_bins.size(); i++) if (fisheye_frame_bins[i]) INFO("[" << i << " = " << fisheye_frame_bins[i] << "], ");
        INFO(" depth frame num differences: ");
        for (size_t i = 0; i < depth_cam_frames_bins.size(); i++) if (depth_cam_frames_bins[i]) INFO("[" << i << " = " << depth_cam_frames_bins[i] << "], ");

        //Consecutive frames are those collected in bin 1
        double gyro_consecutive_frames_percentage = gyro_frame_diff_bins[1] / (double)gyro_frames.size() * 100;
        double accel_consecutive_frames_percentage = accel_frame_diff_bins[1] / (double)accel_frames.size() * 100;
        double fisheye_consecutive_frames_percentage = fisheye_frame_bins[1] / (double)fisheye_timestamp_events.size() * 100;
        double depth_consecutive_frames_percentage = depth_cam_frames_bins[1] / (double)depth_timestamp_events.size() * 100;

        INFO("Gyro frames- consecutive frames  " << gyro_frame_diff_bins[1] << " out of " << gyro_frames.size() << ", consistency rate:" << gyro_consecutive_frames_percentage << "% \n"
            << "Accel frames- consecutive frames  " << accel_frame_diff_bins[1] << " out of " << accel_frames.size() << ", consistency rate:" << accel_consecutive_frames_percentage << "% \n"
            << "Fisheye frames- consecutive frames  " << fisheye_frame_bins[1] << " out of " << fisheye_timestamp_events.size() << ", consistency rate:" << fisheye_consecutive_frames_percentage << "% \n"
            << "Depth frames- consecutive frames  " << depth_cam_frames_bins[1] << " out of " << depth_timestamp_events.size() << ", consistency rate:" << depth_consecutive_frames_percentage << "% \n");

    }
}

TEST_CASE("ZR300 correctly recognizes invalid options", "[live] [DS-device]")
{
    rs::context ctx;
    REQUIRE(ctx.get_device_count() == 1);

    rs::device * dev = ctx.get_device(0);
    REQUIRE(nullptr!=dev);

    const char * name = dev->get_name();
    REQUIRE(ds_names[Intel_ZR300] == dev->get_name());

    int index = 0;
    double val = 0;

    for (int i= (int)rs::option::f200_laser_power; i <= (int)rs::option::sr300_auto_range_lower_threshold; i++)
    {
        index = i;
        try
        {
            rs::option opt = (rs::option)i;
            dev->set_options(&opt,1, &val);
        }
        catch(...)
        {
            REQUIRE(i==index); // Each invoked option must throw exception
        }
    }
}

TEST_CASE( "Test ZR300 streaming mode combinations", "[live] [ZR300] [one-camera]" )
{
    safe_context ctx;

    SECTION( "exactly one device is connected" )
    {
        int device_count = rs_get_device_count(ctx, require_no_error());
        REQUIRE(device_count == 1);
    }

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);

    SECTION( "device name is Intel RealSense ZR300" )
    {
        const char * name = rs_get_device_name(dev, require_no_error());
        REQUIRE(name == std::string("Intel RealSense ZR300"));
    }

    SECTION( "streaming with some configurations" )
    {
        test_streaming(dev, {
            {RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60},
            {RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60},
            {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 60},
            {RS_STREAM_INFRARED2, 640, 480, RS_FORMAT_Y16, 60},
            {RS_STREAM_FISHEYE, 640, 480, RS_FORMAT_RAW8, 60}
        });

        test_streaming(dev, {
            {RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 30},
            {RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 30},
            {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 30},
            {RS_STREAM_INFRARED2, 640, 480, RS_FORMAT_Y16, 30},
            {RS_STREAM_FISHEYE, 640, 480, RS_FORMAT_RAW8, 30}
        });
    }
}

#endif /* !defined(MAKEFILE) || ( defined(LIVE_TEST) && defined(ZR300_TEST) ) */
