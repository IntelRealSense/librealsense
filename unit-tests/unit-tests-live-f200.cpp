// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////
// This set of tests is valid only for the F200 camera //
/////////////////////////////////////////////////////////

#if !defined(MAKEFILE) || ( defined(F200_TEST) && defined(LIVE_TEST) )

#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include "unit-tests-common.h"

#include <sstream>

// due to F200 timestamp spikes it will be not tested in video callbacks mode
inline void test_f200_streaming(rs_device * device, std::initializer_list<stream_mode> modes)
{
    std::map<rs_stream, test_duration> duration_per_stream;
    for(auto & mode : modes)
    {
        duration_per_stream.insert(std::pair<rs_stream, test_duration>(mode.stream, test_duration()));
        rs_enable_stream(device, mode.stream, mode.width, mode.height, mode.format, mode.framerate, require_no_error());
        REQUIRE( rs_is_stream_enabled(device, mode.stream, require_no_error()) == 1 );
    }

    test_wait_for_frames(device, modes, duration_per_stream);

    for(auto & mode : modes)
    {
        rs_disable_stream(device, mode.stream, require_no_error());
        REQUIRE( rs_is_stream_enabled(device, mode.stream, require_no_error()) == 0 );
    }
}

TEST_CASE( "F200 metadata enumerates correctly", "[live] [f200]" )
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

        SECTION( "device name is Intel RealSense F200" )
        {
            const char * name = rs_get_device_name(dev, require_no_error());
            REQUIRE(name == std::string("Intel RealSense F200"));
        }
    }
}

TEST_CASE( "F200 devices support all required options", "[live] [f200]" )
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

        SECTION( "device supports standard picture options and F200 extension options, and nothing else" )
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
                RS_OPTION_F200_LASER_POWER,
                RS_OPTION_F200_ACCURACY,
                RS_OPTION_F200_MOTION_RANGE,
                RS_OPTION_F200_FILTER_OPTION,
                RS_OPTION_F200_CONFIDENCE_THRESHOLD,
                RS_OPTION_F200_DYNAMIC_FPS,
                RS_OPTION_FRAMES_QUEUE_SIZE
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

TEST_CASE( "F200 device extrinsics are within expected parameters", "[live] [f200]" )
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

        SECTION( "depth scale is 1/32 mm" )
        {
            REQUIRE( rs_get_device_depth_scale(dev, require_no_error()) == Approx(1.0f/32000) );
        }
    }
}

///////////////////////////
// Depth streaming tests //
///////////////////////////

TEST_CASE("F200 streams depth (Z16)", "[live] [f200] [one-camera]")
{
    safe_context ctx;
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(rs_get_device_name(dev, require_no_error()) == std::string("Intel RealSense F200"));

    SECTION("F200 streams depth 640x480 (VGA), [15,30,60] fps")
    {
        for (auto & fps : { /*2, 5,*/ 15, 30, 60 })
        {
            INFO("Testing " << fps << " fps");
            test_f200_streaming(dev, { { RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, fps } });
        }
    }

    SECTION("F200 streams depth 640x240 (HVGA), [15,30,60] fps")
    {
        for (auto & fps : {/* 2, 5,*/ 15, 30, 60})
        {
            INFO("Testing " << fps << " fps");
            test_f200_streaming(dev, { { RS_STREAM_DEPTH, 640, 240, RS_FORMAT_Z16, fps } });
        }
    }
}

//////////////////////////////
// Infrared streaming tests //
//////////////////////////////

TEST_CASE("F200 streams infrared (Y16)", "[live] [f200] [one-camera]")
{
    safe_context ctx;
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(rs_get_device_name(dev, require_no_error()) == std::string("Intel RealSense F200"));

    SECTION("F200 streams infrared 640x480 depth (VGA), [30,60,120] fps")
    {
        for (auto & fps : { 30, 60, 120/*, 240, 300*/ })
        {
            INFO("Testing " << fps << " fps")
            test_f200_streaming(dev, { { RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, fps } });
        }
    }

    SECTION("F200 streams infrared 640x240 depth (HVGA), [30,60,120] fps")
    {
        for (auto & fps : { 30, 60, 120/*, 240, 300*/ })
        {
            INFO("Testing " << fps << " fps")
            test_f200_streaming(dev, { { RS_STREAM_INFRARED, 640, 240, RS_FORMAT_Y16, fps } });
        }
    }
}

TEST_CASE( "F200 has no INFRARED2 streaming modes", "[live] [f200]" )
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
        REQUIRE( rs_get_stream_mode_count(dev, RS_STREAM_INFRARED2, require_no_error()) == 0 );
    }
}

/////////////////////
// Streaming tests //
/////////////////////

TEST_CASE( "a single F200 can stream a variety of reasonable streaming mode combinations", "[live] [f200] [one-camera]" )
{
    safe_context ctx;

    SECTION( "exactly one device is connected" )
    {
        int device_count = rs_get_device_count(ctx, require_no_error());
        REQUIRE(device_count == 1);
    }

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);

    SECTION( "device name is Intel RealSense F200" )
    {
        const char * name = rs_get_device_name(dev, require_no_error());
        REQUIRE(name == std::string("Intel RealSense F200"));
    }

    SECTION( "streaming is possible in some reasonable configurations" )
    {
        test_f200_streaming(dev, {
            {RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60}
        });

        test_f200_streaming(dev, {
            {RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60},
            {RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60}
        });

        test_f200_streaming(dev, {
            {RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60},
            {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8, 60}
        });

        test_f200_streaming(dev, {
            {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8, 60},
        });

        test_f200_streaming(dev, {
            {RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60},
            {RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60},
            {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8, 60}
        });
    }
}

#endif /* !defined(MAKEFILE) || ( defined(F200_TEST) && defined(LIVE_TEST) ) */
