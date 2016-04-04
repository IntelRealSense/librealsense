// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

//////////////////////////////////////////////////////////
// This set of tests is valid only for the SR300 camera //
//////////////////////////////////////////////////////////

#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include "unit-tests-common.h"

#include <climits>
#include <sstream>
#include <algorithm>

TEST_CASE( "SR300 metadata enumerates correctly", "[live] [sr300]" )
{
    // Require at least one device to be plugged in
    safe_context ctx;
    const int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count > 0);

    // For each device
    for(int i=0; i<device_count; ++i)
    {
        rs_device * dev = rs_get_device(ctx, 0, require_no_error());
        REQUIRE(dev != nullptr);

        SECTION( "device name is Intel RealSense SR300" )
        {
            const char * name = rs_get_device_name(dev, require_no_error());
            REQUIRE(name == std::string("Intel RealSense SR300"));
        }
    }
}

TEST_CASE( "SR300 devices support all required options", "[live] [sr300]" )
{
    // Require at least one device to be plugged in
    safe_context ctx;
    const int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count > 0);

    // For each device
    for(int i=0; i<device_count; ++i)
    {
        rs_device * dev = rs_get_device(ctx, 0, require_no_error());
        REQUIRE(dev != nullptr);

        SECTION( "device supports standard picture options and SR300 extension options, and nothing else" )
        {
            std::vector<rs_option> supported_options{
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
                RS_OPTION_F200_LASER_POWER,
                RS_OPTION_F200_ACCURACY,
                RS_OPTION_F200_MOTION_RANGE,
                RS_OPTION_F200_FILTER_OPTION,
                RS_OPTION_F200_CONFIDENCE_THRESHOLD,
                RS_OPTION_SR300_DYNAMIC_FPS,
                RS_OPTION_SR300_AUTO_RANGE_ENABLE_MOTION_VERSUS_RANGE,
                RS_OPTION_SR300_AUTO_RANGE_ENABLE_LASER,
                RS_OPTION_SR300_AUTO_RANGE_MIN_MOTION_VERSUS_RANGE,
                RS_OPTION_SR300_AUTO_RANGE_MAX_MOTION_VERSUS_RANGE,
                RS_OPTION_SR300_AUTO_RANGE_START_MOTION_VERSUS_RANGE,
                RS_OPTION_SR300_AUTO_RANGE_MIN_LASER,
                RS_OPTION_SR300_AUTO_RANGE_MAX_LASER,
                RS_OPTION_SR300_AUTO_RANGE_START_LASER,
                RS_OPTION_SR300_AUTO_RANGE_UPPER_THRESHOLD,
                RS_OPTION_SR300_AUTO_RANGE_LOWER_THRESHOLD,
                RS_OPTION_SR300_WAKEUP_DEV_PHASE1_PERIOD,
                RS_OPTION_SR300_WAKEUP_DEV_PHASE1_FPS,
                RS_OPTION_SR300_WAKEUP_DEV_PHASE2_PERIOD,
                RS_OPTION_SR300_WAKEUP_DEV_PHASE2_FPS,
                RS_OPTION_SR300_WAKEUP_DEV_RESET,
                RS_OPTION_SR300_WAKE_ON_USB_REASON,
                RS_OPTION_SR300_WAKE_ON_USB_CONFIDENCE
            };

            for(int i=0; i<RS_OPTION_COUNT; ++i)
            {
                if(std::find(std::begin(supported_options), std::end(supported_options), i) != std::end(supported_options))
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

///////////////////////////////////
// Calibration information tests //
///////////////////////////////////

TEST_CASE( "SR300 device extrinsics are within expected parameters", "[live] [sr300]" )
{
    // Require at least one device to be plugged in
    safe_context ctx;
    const int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count > 0);

    // For each device
    for(int i=0; i<device_count; ++i)
    {
        rs_device * dev = rs_get_device(ctx, 0, require_no_error());
        REQUIRE(dev != nullptr);

        SECTION( "no extrinsic transformation between DEPTH and INFRARED" )
        {
            rs_extrinsics extrin;
            rs_get_device_extrinsics(dev, RS_STREAM_DEPTH, RS_STREAM_INFRARED, &extrin, require_no_error());

            require_identity_matrix(extrin.rotation);
            require_zero_vector(extrin.translation);
        }

        // TODO: Expected depth/color baseline

        SECTION( "depth scale is 0.000125 (by default)" )
        {
            REQUIRE( rs_get_device_depth_scale(dev, require_no_error()) == Approx(0.000125f) );
        }
    }
}

/////////////////////
// Streaming tests //
/////////////////////

inline void test_sr300_streaming(std::initializer_list<stream_mode> modes)
{
    safe_context ctx;
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(rs_get_device_name(dev, require_no_error()) == std::string("Intel RealSense SR300"));

    test_streaming(dev, modes);
}

///////////////////////////
// Depth streaming tests //
///////////////////////////

TEST_CASE( "SR300 streams 640x480 depth", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60}});
}

TEST_CASE( "SR300 streams 640x240 depth", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_DEPTH, 640, 240, RS_FORMAT_Z16, 60}});
}

TEST_CASE( "SR300 streams 640x480 depth (30 fps)", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 30}});
}

TEST_CASE( "SR300 streams 640x240 depth (30 fps)", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_DEPTH, 640, 240, RS_FORMAT_Z16, 30}});
}

TEST_CASE( "SR300 streams 640x240 depth (110 fps)", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_DEPTH, 640, 240, RS_FORMAT_Z16, 110}});
}

///////////////////////////
// Color streaming tests //
///////////////////////////

TEST_CASE( "SR300 streams 1080p color", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_YUYV, 30}});
}

TEST_CASE( "SR300 streams 720p color", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_COLOR, 1280, 720, RS_FORMAT_YUYV, 30}});
}

TEST_CASE( "SR300 streams VGA color", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 30}});
}

TEST_CASE( "SR300 streams 720p color (60 fps)", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_COLOR, 1280, 720, RS_FORMAT_YUYV, 60}});
}

TEST_CASE( "SR300 streams VGA color (60 fps)", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 60}});
}

TEST_CASE( "SR300 streams VGA depth and HD color", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60},
                          {RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_YUYV, 30}});
}

TEST_CASE( "SR300 streams HVGA depth and HD color", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_DEPTH, 640, 240, RS_FORMAT_Z16, 60},
                          {RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_YUYV, 30}});
}

TEST_CASE( "SR300 streams VGA depth and VGA color", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60},
                          {RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 30}});
}

TEST_CASE( "SR300 streams HVGA depth and VGA color", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_DEPTH, 640, 240, RS_FORMAT_Z16, 60},
                          {RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 30}});
}

TEST_CASE( "SR300 streams VGA depth and VGA color (60 fps)", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60},
                          {RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 60}});
}

TEST_CASE( "SR300 streams HVGA depth and VGA color (60 fps)", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_DEPTH, 640, 240, RS_FORMAT_Z16, 60},
                          {RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 60}});
}

//////////////////////////////
// Infrared streaming tests //
//////////////////////////////

TEST_CASE( "SR300 streams 640x480 infrared (30 fps)", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 30}});
}

TEST_CASE( "SR300 streams 640x480 infrared (60 fps)", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 60}});
}

TEST_CASE( "SR300 streams 640x480 infrared (120 fps)", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 120}});
}

TEST_CASE( "SR300 streams 640x480 infrared (200 fps)", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 200}});
}

TEST_CASE( "SR300 streams 640x480 depth and infrared", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60},
                          {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 60}});
}

TEST_CASE( "SR300 streams 640x240 depth and infrared", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_DEPTH, 640, 240, RS_FORMAT_Z16, 60},
                          {RS_STREAM_INFRARED, 640, 240, RS_FORMAT_Y16, 60}});
}

TEST_CASE( "SR300 streams 640x240 depth and infrared (110 fps)", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_DEPTH, 640, 240, RS_FORMAT_Z16, 110},
                          {RS_STREAM_INFRARED, 640, 240, RS_FORMAT_Y16, 110}});
}

TEST_CASE( "SR300 streams 640x480 depth, infrared, and color", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60},
                          {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 60},
                          {RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60}});
}

TEST_CASE( "SR300 streams 640x240 depth and infrared (110 fps), and 1080P color (30 fps)", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_DEPTH, 640, 240, RS_FORMAT_Z16, 110},
                          {RS_STREAM_INFRARED, 640, 240, RS_FORMAT_Y16, 110},
                          {RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_RGB8, 30}});
}

TEST_CASE( "SR300 streams 640x480 infrared (200 fps), and VGA color (60 fps)", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 200},
                          {RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60}});
}

TEST_CASE( "SR300 streams 640x480 infrared (200 fps), and 1080P color (30 fps)", "[live] [sr300] [one-camera]" )
{
    test_sr300_streaming({{RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 200},
                          {RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_RGB8, 30}});
}

/////////////
// Options //
/////////////

enum { BEFORE_START_DEVICE = 1, AFTER_START_DEVICE = 2 };
inline void test_sr300_option(rs_option option, std::initializer_list<int> values, int when)
{
    safe_context ctx;
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(rs_get_device_name(dev, require_no_error()) == std::string("Intel RealSense SR300"));

    if(when & BEFORE_START_DEVICE)
    {
        test_option(dev, option, values, {});
    }

    if(when & AFTER_START_DEVICE)
    {
        rs_enable_stream_preset(dev, RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY, require_no_error());
        rs_start_device(dev, require_no_error());

        // Currently, setting/getting options immediately after streaming frequently raises hardware errors
        // todo - Internally block or retry failed calls within the first few seconds after streaming
        std::this_thread::sleep_for(std::chrono::seconds(1));
        test_option(dev, option, values, {});
    }
}

TEST_CASE( "SR300 supports RS_OPTION_F200_LASER_POWER", "[live] [sr300]" )
{
    test_sr300_option(RS_OPTION_F200_LASER_POWER, {0, 1, 2, 4, 8, 15}, AFTER_START_DEVICE);
}

TEST_CASE( "SR300 supports RS_OPTION_F200_ACCURACY", "[live] [sr300]" )
{
    test_sr300_option(RS_OPTION_F200_ACCURACY, { 1, 2, 3}, AFTER_START_DEVICE);
}

TEST_CASE( "SR300 supports RS_OPTION_F200_MOTION_RANGE", "[live] [sr300]" )
{
    test_sr300_option(RS_OPTION_F200_MOTION_RANGE, {0, 1, 8, 25, 50, 100}, AFTER_START_DEVICE);
}

TEST_CASE( "SR300 supports RS_OPTION_F200_FILTER_OPTION", "[live] [sr300]" )
{
    test_sr300_option(RS_OPTION_F200_FILTER_OPTION, {0, 1, 2, 3, 4, 5, 6, 7}, AFTER_START_DEVICE);
}

TEST_CASE( "SR300 supports RS_OPTION_F200_CONFIDENCE_THRESHOLD", "[live] [sr300]" )
{
    test_sr300_option(RS_OPTION_F200_LASER_POWER, {0, 1, 2, 4, 8, 15}, AFTER_START_DEVICE);
}

//////////////////////////////////////////
// Stop, reconfigure, and restart tests //
//////////////////////////////////////////

TEST_CASE( "a single SR300 can stream a variety of reasonable streaming mode combinations", "[live] [sr300] [one-camera]" )
{
    safe_context ctx;

    SECTION( "exactly one device is connected" )
    {
        int device_count = rs_get_device_count(ctx, require_no_error());
        REQUIRE(device_count == 1);
    }

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);

    SECTION( "device name is Intel RealSense SR300" )
    {
        const char * name = rs_get_device_name(dev, require_no_error());
        REQUIRE(name == std::string("Intel RealSense SR300"));
    }

    SECTION( "streaming is possible in some reasonable configurations" )
    {
        test_streaming(dev, {
            {RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60}
        });

        test_streaming(dev, {
            {RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60},
            {RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60}
        });

        test_streaming(dev, {
            {RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60},
            {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 60}
        });

        test_streaming(dev, {
            {RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60},
            {RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60},
            {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 60}
        });
    }
}

inline void test_options(rs_device * device, rs_option* option_list, size_t options, std::vector<double> good_values, std::vector<double> bad_values, std::vector<double> &ret_values, const std::string & expected_success_msg, const std::string & expected_error_msg, bool bWrite)
{    
    if (bWrite)
    {
        // Test setting good values
        if (good_values.size() == options)
        {
            if (expected_success_msg.size())
                rs_set_device_options(device, option_list, (int)options, good_values.data(), require_error(expected_success_msg));
            else
                rs_set_device_options(device, option_list, (int)options, good_values.data(), require_no_error());
        }

        if (bad_values.size() == options)
        {
            if (expected_error_msg.size())
                rs_set_device_options(device, option_list, (int)options, bad_values.data(), require_error(expected_error_msg));
            else
                rs_set_device_options(device, option_list, (int)options, bad_values.data(), require_no_error());
        }
    }
    else // Read command
    {
        std::vector<double> vretVal;
        vretVal.resize(options);
        if (expected_success_msg.size())
            rs_get_device_options(device, option_list, (int)options, vretVal.data(), require_error(expected_success_msg));
        else
            rs_get_device_options(device, option_list, (int)options, vretVal.data(), require_no_error());

        // Results to be returned
        ret_values = vretVal;       
    }
}

inline void test_sr300_command(rs_device *dev, std::vector<rs_option> options_list,
    std::vector<double> good_values, std::vector<double> bad_values, std::vector<double>& ret_values, const std::string& expected_success_msg, const std::string& expected_error_msg, int when, bool write_cmd)
{    
    REQUIRE(dev != nullptr);

    for (auto opt : options_list)
    {
        REQUIRE(rs_device_supports_option(dev, opt, require_no_error()) == 1);
    }

    if (when & BEFORE_START_DEVICE)
    {
        test_options(dev, options_list.data(), options_list.size(), good_values, bad_values, ret_values, expected_success_msg, expected_error_msg, write_cmd);
    }

    if (when & AFTER_START_DEVICE)
    {
        rs_enable_stream_preset(dev, RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY, require_no_error());
        rs_start_device(dev, require_no_error());

        // Currently, setting/getting options immediately after streaming frequently raises hardware errors
        // todo - Internally block or retry failed calls within the first few seconds after streaming
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        test_options(dev, options_list.data(), options_list.size(), good_values, bad_values, ret_values, expected_success_msg, expected_error_msg, write_cmd);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        rs_stop_device(dev, require_no_error());
    }
}

TEST_CASE("SR300 Wakeup over USB function", "[live] [sr300]")
{
    std::vector<rs_option> vSetWakeupDevCmd;
    std::vector<rs_option> vGetWakeUpDevCmd;

    std::vector<std::vector<double>> vGoodParams;
    std::vector<std::vector<double>> vBadParams;
    std::vector<double> vRetValues;

    std::string strSuccessMsg = ("");
    std::string strErrorMsg = ("missing/invalid wake_up command parameters");

    safe_context ctx;
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);

    SECTION("Minimal Main Success Scenario")
    {
        vSetWakeupDevCmd = { RS_OPTION_SR300_WAKEUP_DEV_PHASE1_PERIOD, RS_OPTION_SR300_WAKEUP_DEV_PHASE1_FPS, RS_OPTION_SR300_WAKEUP_DEV_PHASE2_PERIOD, RS_OPTION_SR300_WAKEUP_DEV_PHASE2_FPS };
        vGetWakeUpDevCmd = { RS_OPTION_SR300_WAKE_ON_USB_REASON, RS_OPTION_SR300_WAKE_ON_USB_CONFIDENCE };

        //phase1Period  phase1FPS   phase2Period phase2FPS
        vGoodParams = { { 500, 2, 200, 3 } };
        vBadParams = {};              // second and forth parameters are invalid
    }

    SECTION("Main Success Scenario Extended")
    {
        vSetWakeupDevCmd = { RS_OPTION_SR300_WAKEUP_DEV_PHASE1_PERIOD, RS_OPTION_SR300_WAKEUP_DEV_PHASE1_FPS, RS_OPTION_SR300_WAKEUP_DEV_PHASE2_PERIOD, RS_OPTION_SR300_WAKEUP_DEV_PHASE2_FPS };
        vGetWakeUpDevCmd = { RS_OPTION_SR300_WAKE_ON_USB_REASON, RS_OPTION_SR300_WAKE_ON_USB_CONFIDENCE };

        //phase1Period  phase1FPS   phase2Period phase2FPS
        vGoodParams = {
            { 500, 2, 200, 3 },
            { 5000, 1, 456, 2 } };
        vBadParams = {
            { 0xffffffff, 0, 3, 6 },       // first parameter is invalid
            { 1, 32, 3, 6 },               // second parameter is invalid : enum in range [0-3]
            { 1, 2, 3, 6 },                // fourth parameter is invalid : enum in range [0-3]
            { 1, 2, 3, 6 } };              // second and forth parameters are invalid
    }

    SECTION("Negative Tests: Ill-formed commands")
    {
        SECTION("WAKEUP_DEV_PHASE1_PERIOD option is undefined")
        {
            vSetWakeupDevCmd = { RS_OPTION_SR300_WAKEUP_DEV_PHASE2_PERIOD, RS_OPTION_SR300_WAKEUP_DEV_PHASE1_FPS, RS_OPTION_SR300_WAKEUP_DEV_PHASE2_PERIOD, RS_OPTION_SR300_WAKEUP_DEV_PHASE2_FPS };
        }

        SECTION("number of command options does not correspond to the number of assignment values")
        {
            vSetWakeupDevCmd = { RS_OPTION_SR300_WAKEUP_DEV_PHASE1_PERIOD, RS_OPTION_SR300_WAKEUP_DEV_PHASE2_PERIOD, RS_OPTION_SR300_WAKEUP_DEV_PHASE1_FPS, RS_OPTION_SR300_WAKEUP_DEV_PHASE2_PERIOD, RS_OPTION_SR300_WAKEUP_DEV_PHASE2_FPS };
        }

        strSuccessMsg = ("missing/invalid wake_up command parameters");    // Eventhought the parameters are correct we'd expect the test to fail due to invalid command options specification

        vGetWakeUpDevCmd = { RS_OPTION_SR300_WAKE_ON_USB_REASON, RS_OPTION_SR300_WAKE_ON_USB_CONFIDENCE };
        vGoodParams = { { 500, 2, 200, 3 } };
        vBadParams = {};
    }

    // Apply set command in different scenarios
    for (auto &data : vGoodParams)
    {
        test_sr300_command(dev, vSetWakeupDevCmd, data, {}, vRetValues, strSuccessMsg, strErrorMsg, BEFORE_START_DEVICE, true);
        test_sr300_command(dev, vSetWakeupDevCmd, data, {}, vRetValues, strSuccessMsg, strErrorMsg, AFTER_START_DEVICE, true);
    }

    for (auto &data : vBadParams)
    {
        test_sr300_command(dev, vSetWakeupDevCmd, {}, data, vRetValues, strSuccessMsg, strErrorMsg, BEFORE_START_DEVICE, true);
        test_sr300_command(dev, vSetWakeupDevCmd, {}, data, vRetValues, strSuccessMsg, strErrorMsg, AFTER_START_DEVICE, true);
    }

    // Revert to original messages
    strSuccessMsg = ("");
    strErrorMsg = ("missing/invalid wake_up command parameters");
    
    test_sr300_command(dev, vGetWakeUpDevCmd, {}, {}, vRetValues, strSuccessMsg, strErrorMsg, BEFORE_START_DEVICE, false);
    test_sr300_command(dev, vGetWakeUpDevCmd, {}, {}, vRetValues, strSuccessMsg, strErrorMsg, AFTER_START_DEVICE, false);

    REQUIRE(((vRetValues[0] == Approx(1.0)) || ((int)vRetValues[0] == Approx(0))) == true );
    REQUIRE(((vRetValues[1] >= 0) && (vRetValues[1] <= 100) && ( 0 == (int)vRetValues[1]%10))== true);
    
}
