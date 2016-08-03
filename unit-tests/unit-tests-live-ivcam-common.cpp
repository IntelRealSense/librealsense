// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////
// This set of tests is valid only for the R200 camera //
/////////////////////////////////////////////////////////

#if !defined(MAKEFILE)

#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include "unit-tests-common.h"

#include <climits>
#include <sstream>

TEST_CASE( "R200 metadata enumerates correctly", "[live] [r200]" )
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

        SECTION( "device name is Intel RealSense R200" )
        {
            const char * name = rs_get_device_name(dev, require_no_error());
            REQUIRE(name == std::string("Intel RealSense R200"));
        }

        SECTION( "device serial number has ten decimal digits" )
        {
            const char * serial = rs_get_device_serial(dev, require_no_error());
            REQUIRE(strlen(serial) == 10);
            for(int i=0; i<10; ++i) REQUIRE(isdigit(serial[i]));
        }
    }
}

TEST_CASE( "R200 devices support all required options", "[live] [r200]" )
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

        SECTION( "device supports standard picture options and R200 extension options, and nothing else" )
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
                RS_OPTION_R200_DEPTH_CONTROL_LR_THRESHOLD
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

TEST_CASE( "R200 device extrinsics are within expected parameters", "[live] [r200]" )
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

        SECTION( "only x-axis translation (~70 mm) between DEPTH and INFRARED2" )
        {
            rs_extrinsics extrin;
            rs_get_device_extrinsics(dev, RS_STREAM_DEPTH, RS_STREAM_INFRARED2, &extrin, require_no_error());

            require_identity_matrix(extrin.rotation);
            REQUIRE( extrin.translation[0] < -0.06f ); // Some variation is allowed, but should report at least 60 mm in all cases
            REQUIRE( extrin.translation[0] > -0.08f ); // Some variation is allowed, but should report at most 80 mm in all cases
            REQUIRE( extrin.translation[1] == 0.0f );
            REQUIRE( extrin.translation[2] == 0.0f );
        }

        SECTION( "only translation between DEPTH and RECTIFIED_COLOR" )
        {
            rs_extrinsics extrin;
            rs_get_device_extrinsics(dev, RS_STREAM_DEPTH, RS_STREAM_RECTIFIED_COLOR, &extrin, require_no_error());

            require_identity_matrix(extrin.rotation);
        }

        SECTION( "depth scale is 0.001 (by default)" )
        {
            REQUIRE( rs_get_device_depth_scale(dev, require_no_error()) == 0.001f );
        }
    }
}

TEST_CASE( "R200 infrared2 streaming modes exactly match infrared streaming modes", "[live] [r200]" )
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

        // Require that there are a nonzero amount of infrared modes, and that infrared2 has the same number of modes
        const int infrared_mode_count = rs_get_stream_mode_count(dev, RS_STREAM_INFRARED, require_no_error());
        REQUIRE( infrared_mode_count > 0 );
        REQUIRE( rs_get_stream_mode_count(dev, RS_STREAM_INFRARED2, require_no_error()) == infrared_mode_count );

        // For each streaming mode
        for(int j=0; j<infrared_mode_count; ++j)
        {
            // Require that INFRARED and INFRARED2 streaming modes are exactly identical
            int infrared_width = 0, infrared_height = 0, infrared_framerate = 0; rs_format infrared_format = RS_FORMAT_ANY;
            rs_get_stream_mode(dev, RS_STREAM_INFRARED, j, &infrared_width, &infrared_height, &infrared_format, &infrared_framerate, require_no_error());

            int infrared2_width = 0, infrared2_height = 0, infrared2_framerate = 0; rs_format infrared2_format = RS_FORMAT_ANY;
            rs_get_stream_mode(dev, RS_STREAM_INFRARED2, j, &infrared2_width, &infrared2_height, &infrared2_format, &infrared2_framerate, require_no_error());

            REQUIRE( infrared_width == infrared2_width );
            REQUIRE( infrared_height == infrared2_height );
            REQUIRE( infrared_format == infrared2_format );
            REQUIRE( infrared_framerate == infrared2_framerate );

            // Require that the intrinsics for these streaming modes match exactly
            rs_enable_stream(dev, RS_STREAM_INFRARED, infrared_width, infrared_height, infrared_format, infrared_framerate, require_no_error());
            rs_enable_stream(dev, RS_STREAM_INFRARED2, infrared2_width, infrared2_height, infrared2_format, infrared2_framerate, require_no_error());

            REQUIRE( rs_get_stream_format(dev, RS_STREAM_INFRARED, require_no_error()) == rs_get_stream_format(dev, RS_STREAM_INFRARED2, require_no_error()) );
            REQUIRE( rs_get_stream_framerate(dev, RS_STREAM_INFRARED, require_no_error()) == rs_get_stream_framerate(dev, RS_STREAM_INFRARED2, require_no_error()) );

            rs_intrinsics infrared_intrin = {}, infrared2_intrin = {};
            rs_get_stream_intrinsics(dev, RS_STREAM_INFRARED, &infrared_intrin, require_no_error());
            rs_get_stream_intrinsics(dev, RS_STREAM_INFRARED2, &infrared2_intrin, require_no_error());
            REQUIRE( infrared_intrin.width  == infrared_intrin.width  );
            REQUIRE( infrared_intrin.height == infrared_intrin.height );
            REQUIRE( infrared_intrin.ppx    == infrared_intrin.ppx    );
            REQUIRE( infrared_intrin.ppy    == infrared_intrin.ppy    );
            REQUIRE( infrared_intrin.fx     == infrared_intrin.fx     );
            REQUIRE( infrared_intrin.fy     == infrared_intrin.fy     );
            REQUIRE( infrared_intrin.model  == infrared_intrin.model  );
            for(int k=0; k<5; ++k) REQUIRE( infrared_intrin.coeffs[k]  == infrared_intrin.coeffs[k] );
        }
    }
}

/////////////////////
// Streaming tests //
/////////////////////

inline void test_r200_streaming(std::initializer_list<stream_mode> modes)
{
    safe_context ctx;   
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(rs_get_device_name(dev, require_no_error()) == std::string("Intel RealSense R200"));

    test_streaming(dev, modes);
}

///////////////////////////
// Depth streaming tests //
///////////////////////////

TEST_CASE( "R200 streams 480x360 depth", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60}});
}

TEST_CASE( "R200 streams 628x468 depth", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 628, 468, RS_FORMAT_Z16, 60}});
}

TEST_CASE( "R200 streams 320x240 depth", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 320, 240, RS_FORMAT_Z16, 60}});
}

TEST_CASE( "R200 streams 480x360 depth (30 fps)", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 30}});
}

TEST_CASE( "R200 streams 628x468 depth (30 fps)", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 628, 468, RS_FORMAT_Z16, 30}});
}

TEST_CASE( "R200 streams 320x240 depth (30 fps)", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 320, 240, RS_FORMAT_Z16, 30}});
}

TEST_CASE( "R200 streams 480x360 depth (90 fps)", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 90}});
}

TEST_CASE( "R200 streams 628x468 depth (90 fps)", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 628, 468, RS_FORMAT_Z16, 90}});
}

TEST_CASE( "R200 streams 320x240 depth (90 fps)", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 320, 240, RS_FORMAT_Z16, 90}});
}

///////////////////////////
// Color streaming tests //
///////////////////////////

TEST_CASE( "R200 streams HD color", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_YUYV, 30}});
}

TEST_CASE( "R200 streams VGA color", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 30}});
}

TEST_CASE( "R200 streams VGA color (60 fps)", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 60}});
}

TEST_CASE( "R200 streams 480x360 depth and HD color", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60},
                         {RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_YUYV, 30}});
}

TEST_CASE( "R200 streams 628x468 depth and HD color", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 628, 468, RS_FORMAT_Z16, 60},
                         {RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_YUYV, 30}});
}

TEST_CASE( "R200 streams 320x240 depth and HD color", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 320, 240, RS_FORMAT_Z16, 60},
                         {RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_YUYV, 30}});
}

TEST_CASE( "R200 streams 480x360 depth and VGA color", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60},
                         {RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 30}});
}

TEST_CASE( "R200 streams 628x468 depth and VGA color", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 628, 468, RS_FORMAT_Z16, 60},
                         {RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 30}});
}

TEST_CASE( "R200 streams 320x240 depth and VGA color", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 320, 240, RS_FORMAT_Z16, 60},
                         {RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 30}});
}

TEST_CASE( "R200 streams 480x360 depth and VGA color (60 fps)", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60},
                         {RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 60}});
}

TEST_CASE( "R200 streams HD Raw10", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_RAW10, 30}});
}

//////////////////////////////
// Infrared streaming tests //
//////////////////////////////

TEST_CASE( "R200 streams 640x480 infrared (left 8 bit)", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8, 60}});
}

TEST_CASE( "R200 streams 640x480 infrared (left 16 bit)", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 60}});
}

TEST_CASE( "R200 streams 640x480 infrared (right 8 bit)", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_INFRARED2, 640, 480, RS_FORMAT_Y8, 60}});
}

TEST_CASE( "R200 streams 640x480 infrared (right 16 bit)", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_INFRARED2, 640, 480, RS_FORMAT_Y16, 60}});
}

TEST_CASE( "R200 streams 640x480 infrared (left+right 8 bit)", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8, 60},
                         {RS_STREAM_INFRARED2, 640, 480, RS_FORMAT_Y8, 60}});
}

TEST_CASE( "R200 streams 640x480 infrared (left+right 16 bit)", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 60},
                         {RS_STREAM_INFRARED2, 640, 480, RS_FORMAT_Y16, 60}});
}

TEST_CASE( "R200 streams 480x360 depth and 492x372 infrared (left+right 16 bit)", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60},
                         {RS_STREAM_INFRARED, 492, 372, RS_FORMAT_Y16, 60},
                         {RS_STREAM_INFRARED2, 492, 372, RS_FORMAT_Y16, 60}});
}

TEST_CASE( "R200 streams 480x360 depth, VGA color, and 492x372 infrared", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60},
                         {RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 30},
                         {RS_STREAM_INFRARED, 492, 372, RS_FORMAT_Y16, 60},
                         {RS_STREAM_INFRARED2, 492, 372, RS_FORMAT_Y16, 60}});
}

TEST_CASE( "R200 streams 628x468 depth, VGA color, and 640x480 infrared", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 628, 468, RS_FORMAT_Z16, 60},
                         {RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 30},
                         {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 60},
                         {RS_STREAM_INFRARED2, 640, 480, RS_FORMAT_Y16, 60}});
}

TEST_CASE( "R200 streams 320x240 depth, VGA color, and 332x252 infrared", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 320, 240, RS_FORMAT_Z16, 60},
                         {RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 30},
                         {RS_STREAM_INFRARED, 332, 252, RS_FORMAT_Y16, 60},
                         {RS_STREAM_INFRARED2, 332, 252, RS_FORMAT_Y16, 60}});
}

//////////////////////////////
// Cropped and padded modes //
//////////////////////////////

TEST_CASE( "R200 streams 640x480 depth and infrared", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60},
                         {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8, 60},
                         {RS_STREAM_INFRARED2, 640, 480, RS_FORMAT_Y8, 60}});
}

TEST_CASE( "R200 streams 628x468 depth and infrared", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 628, 468, RS_FORMAT_Z16, 60},
                         {RS_STREAM_INFRARED, 628, 468, RS_FORMAT_Y8, 60},
                         {RS_STREAM_INFRARED2, 628, 468, RS_FORMAT_Y8, 60}});
}

TEST_CASE( "R200 streams 492x372 depth and infrared", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 492, 372, RS_FORMAT_Z16, 60},
                         {RS_STREAM_INFRARED, 492, 372, RS_FORMAT_Y8, 60},
                         {RS_STREAM_INFRARED2, 492, 372, RS_FORMAT_Y8, 60}});
}

TEST_CASE( "R200 streams 480x360 depth and infrared", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60},
                         {RS_STREAM_INFRARED, 480, 360, RS_FORMAT_Y8, 60},
                         {RS_STREAM_INFRARED2, 480, 360, RS_FORMAT_Y8, 60}});
}

TEST_CASE( "R200 streams 332x252 depth and infrared", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 332, 252, RS_FORMAT_Z16, 60},
                         {RS_STREAM_INFRARED, 332, 252, RS_FORMAT_Y8, 60},
                         {RS_STREAM_INFRARED2, 332, 252, RS_FORMAT_Y8, 60}});
}

TEST_CASE( "R200 streams 320x240 depth and infrared", "[live] [r200] [one-camera]" )
{
    test_r200_streaming({{RS_STREAM_DEPTH, 320, 240, RS_FORMAT_Z16, 60},
                         {RS_STREAM_INFRARED, 320, 240, RS_FORMAT_Y8, 60},
                         {RS_STREAM_INFRARED2, 320, 240, RS_FORMAT_Y8, 60}});
}

/////////////
// Options //
/////////////

enum { BEFORE_START_DEVICE = 1, AFTER_START_DEVICE = 2 };
inline void test_r200_option(rs_option option, std::initializer_list<int> values, int when)
{
    safe_context ctx;   
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(rs_get_device_name(dev, require_no_error()) == std::string("Intel RealSense R200"));

    if(when & BEFORE_START_DEVICE)
    {
        test_option(dev, option, values, {});
    }

    if(when & AFTER_START_DEVICE)
    {
        rs_enable_stream_preset(dev, RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY, require_no_error());
        rs_start_device(dev, rs_source::RS_SOURCE_VIDEO, require_no_error());

        // Currently, setting/getting options immediately after streaming frequently raises hardware errors
        // todo - Internally block or retry failed calls within the first few seconds after streaming
        std::this_thread::sleep_for(std::chrono::seconds(1));
        test_option(dev, option, values, {});
    }
}

TEST_CASE( "R200 supports RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED", "[live] [r200]" )
{
    test_r200_option(RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED, {0, 1}, BEFORE_START_DEVICE | AFTER_START_DEVICE);
}

TEST_CASE( "R200 supports RS_OPTION_R200_LR_GAIN", "[live] [r200]" )
{
    test_r200_option(RS_OPTION_R200_LR_GAIN, {100, 200, 400, 800, 1600}, BEFORE_START_DEVICE | AFTER_START_DEVICE); // Gain percentage   
}

TEST_CASE( "R200 supports RS_OPTION_R200_LR_EXPOSURE", "[live] [r200]" )
{
    test_r200_option(RS_OPTION_R200_LR_EXPOSURE, {40, 80, 160}, BEFORE_START_DEVICE | AFTER_START_DEVICE); // Tenths of milliseconds   
}

// Note: The R200 firmware has some complications regarding emitter state before the device has been started
// The emitter will never be on if the device is not streaming, but the firmware will remember and respect any
// specified preferences for emitter enabled that are specified prior to streaming.

TEST_CASE( "R200 emitter defaults to off if depth is not enabled/streamed", "[live] [r200]" )
{
    safe_context ctx;   
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(rs_get_device_name(dev, require_no_error()) == std::string("Intel RealSense R200"));

    // Emitter enabled defaults to false
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);

    // Enabling non-depth streams does not change the emitter's state
    rs_enable_stream_preset(dev, RS_STREAM_COLOR, RS_PRESET_BEST_QUALITY, require_no_error());
    rs_enable_stream_preset(dev, RS_STREAM_INFRARED, RS_PRESET_BEST_QUALITY, require_no_error());
    rs_enable_stream_preset(dev, RS_STREAM_INFRARED2, RS_PRESET_BEST_QUALITY, require_no_error());
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);

    // Starting the device does not change the emitter's state
    rs_start_device(dev, rs_source::RS_SOURCE_VIDEO, require_no_error());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);
}

TEST_CASE( "R200 emitter defaults to on if depth is enabled/streamed", "[live] [r200]" )
{
    safe_context ctx;   
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(rs_get_device_name(dev, require_no_error()) == std::string("Intel RealSense R200"));

    // Emitter enabled defaults to false
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);

    // Enabling depth stream causes the emitter to be enabled
    rs_enable_stream_preset(dev, RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY, require_no_error());
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 1);

    // Starting the device does not change the emitter's state
    rs_start_device(dev, rs_source::RS_SOURCE_VIDEO, require_no_error());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 1);
}

TEST_CASE( "R200 emitter can be enabled even if depth is not enabled/streamed", "[live] [r200]" )
{
    safe_context ctx;   
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(rs_get_device_name(dev, require_no_error()) == std::string("Intel RealSense R200"));

    // Emitter enabled defaults to false
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);

    // Enabling non-depth streams does not change the emitter's state
    rs_enable_stream_preset(dev, RS_STREAM_COLOR, RS_PRESET_BEST_QUALITY, require_no_error());
    rs_enable_stream_preset(dev, RS_STREAM_INFRARED, RS_PRESET_BEST_QUALITY, require_no_error());
    rs_enable_stream_preset(dev, RS_STREAM_INFRARED2, RS_PRESET_BEST_QUALITY, require_no_error());
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);

    // The emitter can be turned on even though no depth is streamed
    rs_set_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, 1, require_no_error());
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 1);

    // Starting the device does not change the emitter's state
    rs_start_device(dev, rs_source::RS_SOURCE_VIDEO, require_no_error());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 1);
}

TEST_CASE( "R200 emitter can be turned off even if depth is enabled/streamed", "[live] [r200]" )
{
    safe_context ctx;   
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(rs_get_device_name(dev, require_no_error()) == std::string("Intel RealSense R200"));

    // Emitter enabled defaults to false
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);

    // Enabling depth stream causes the emitter to be enabled
    rs_enable_stream_preset(dev, RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY, require_no_error());
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 1);

    // The emitter can be turned off even though depth is streamed
    rs_set_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, 0, require_no_error());
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);

    // Starting the device does not change the emitter's state
    rs_start_device(dev, rs_source::RS_SOURCE_VIDEO, require_no_error());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);
}

TEST_CASE( "R200 emitter can be turned on and off after streaming has begun", "[live] [r200]" )
{
    safe_context ctx;   
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(rs_get_device_name(dev, require_no_error()) == std::string("Intel RealSense R200"));

    // The emitter defaults to on when depth is streamed
    rs_enable_stream_preset(dev, RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY, require_no_error());
    rs_start_device(dev, rs_source::RS_SOURCE_VIDEO, require_no_error());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 1);

    // The emitter can be turned off
    rs_set_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, 0, require_no_error());
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);

    // The emitter can be turned back on
    rs_set_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, 1, require_no_error());
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 1);
}

TEST_CASE( "R200 supports RS_OPTION_R200_DEPTH_UNITS", "[live] [r200]" )
{
    safe_context ctx;   
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(rs_get_device_name(dev, require_no_error()) == std::string("Intel RealSense R200"));

    // By default, depth unit is 1000 micrometers (1 mm)
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_DEPTH_UNITS, require_no_error()) == 1000);
    REQUIRE(rs_get_device_depth_scale(dev, require_no_error()) == 0.001f);

    for(int value : {100, 500, 1000, 2000, 10000})
    {
        // Set depth units (specified in micrometers) and verify that depth scale (specified in meters) changes appropriately
        rs_set_device_option(dev, RS_OPTION_R200_DEPTH_UNITS, value, require_no_error());
        REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_DEPTH_UNITS, require_no_error()) == value);
        REQUIRE(rs_get_device_depth_scale(dev, require_no_error()) == (float)value/1000000);
    }
}

TEST_CASE( "R200 supports RS_OPTION_R200_DEPTH_CLAMP_MIN", "[live] [r200]" )
{
    test_r200_option(RS_OPTION_R200_DEPTH_CLAMP_MIN, {0, 500, 1000, 2000}, BEFORE_START_DEVICE);
}

TEST_CASE( "R200 supports RS_OPTION_R200_DEPTH_CLAMP_MAX", "[live] [r200]" )
{
    test_r200_option(RS_OPTION_R200_DEPTH_CLAMP_MAX, {500, 1000, 2000, USHRT_MAX}, BEFORE_START_DEVICE);
}

//////////////////////////////////////////
// Stop, reconfigure, and restart tests //
//////////////////////////////////////////

TEST_CASE( "a single R200 can stream a variety of reasonable streaming mode combinations", "[live] [r200] [one-camera]" )
{
    safe_context ctx;

    SECTION( "exactly one device is connected" )
    {
        int device_count = rs_get_device_count(ctx, require_no_error());
        REQUIRE(device_count == 1);
    }

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);

    SECTION( "device name is Intel RealSense R200" )
    {
        const char * name = rs_get_device_name(dev, require_no_error());
        REQUIRE(name == std::string("Intel RealSense R200"));
    }

    SECTION( "streaming is possible in some reasonable configurations" )
    {
        SECTION( "streaming DEPTH 480, 360, RS_FORMAT_Z16, 60" )
        {
            test_streaming(dev, {
                {RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60}
            });
        }

        SECTION( "streaming [DEPTH,480,360,Z16,60] [COLOR,480,360,RGB8,60]" )
        {
            test_streaming(dev, {
                {RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60},
                {RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60}
            });
        }

        SECTION( "streaming [DEPTH,480,360,Z16,60] [IR,480,360,Y8,60]" )
        {
            test_streaming(dev, {
                {RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60},
                {RS_STREAM_INFRARED, 480, 360, RS_FORMAT_Y8, 60}
            });
        }

        SECTION( "streaming [IR,492,372,Y16,60] [IR2,492,372,Y16,60]" )
        {
            test_streaming(dev, {
                {RS_STREAM_INFRARED, 492, 372, RS_FORMAT_Y16, 60},
                {RS_STREAM_INFRARED2, 492, 372, RS_FORMAT_Y16, 60}
            });
        }

        SECTION( "streaming [DEPTH,480,360,Z16,60] [COLOR,640,480,RGB8,60] [IR,480,360,Y8,60] [IR2,480,360,Y8,60]" )
        {
            test_streaming(dev, {
                {RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60},
                {RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60},
                {RS_STREAM_INFRARED, 480, 360, RS_FORMAT_Y8, 60},
                {RS_STREAM_INFRARED2, 480, 360, RS_FORMAT_Y8, 60}
            });
        }
    }
}

TEST_CASE( "streaming five configurations sequentionally", "[live] [r200] [one-camera]" )
{
    safe_context ctx;

    int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);

    const char * name = rs_get_device_name(dev, require_no_error());
    REQUIRE(name == std::string("Intel RealSense R200"));

    test_streaming(dev, {
        {RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60}
    });

    test_streaming(dev, {
        {RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60},
        {RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60}
    });

    test_streaming(dev, {
        {RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60}
    });

    test_streaming(dev, {
        {RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60},
        {RS_STREAM_INFRARED, 480, 360, RS_FORMAT_Y8, 60}
    });

    test_streaming(dev, {
        {RS_STREAM_INFRARED, 492, 372, RS_FORMAT_Y16, 60},
        {RS_STREAM_INFRARED2, 492, 372, RS_FORMAT_Y16, 60}
    });

    test_streaming(dev, {
        {RS_STREAM_INFRARED, 492, 372, RS_FORMAT_Y16, 60},
        {RS_STREAM_INFRARED2, 492, 372, RS_FORMAT_Y16, 60}
    });

    test_streaming(dev, {
        {RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60},
        {RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60},
        {RS_STREAM_INFRARED, 480, 360, RS_FORMAT_Y8, 60},
        {RS_STREAM_INFRARED2, 480, 360, RS_FORMAT_Y8, 60}
    });

}

#endif /* !defined(MAKEFILE) */