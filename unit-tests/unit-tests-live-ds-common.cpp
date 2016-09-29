// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////
// This set of tests is valid only for the DS-device camera //
/////////////////////////////////////////////////////////

#if !defined(MAKEFILE) || ( defined(LR200_TEST) || defined(R200_TEST) || defined(ZR300_TEST) )

#define CATCH_CONFIG_MAIN

#include <climits>
#include <sstream>

#include "unit-tests-live-ds-common.h"


// noexcept is not accepted by Visual Studio 2013 yet, but noexcept(false) is require on throwing destructors on gcc and clang
// It is normally advisable not to throw in a destructor, however, this usage is safe for require_error/require_no_error because
// they will only ever be created as temporaries immediately before being passed to a C ABI function. All parameters and return
// types are vanilla C types, and thus nothrow-copyable, and the function itself cannot throw because it is a C ABI function.
// Therefore, when a temporary require_error/require_no_error is destructed immediately following one of these C ABI function
// calls, we should not have any exceptions in flight, and can freely throw (perhaps indirectly by calling Catch's REQUIRE() 
// macro) to indicate postcondition violations.
#ifdef WIN32
#define NOEXCEPT_FALSE
#else
#define NOEXCEPT_FALSE noexcept(false)
#endif


TEST_CASE("DS-dev metadata enumerates correctly", "[live] [DS-device]")
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

        SECTION("device name identification ")
        {
            REQUIRE(std::any_of(ds_names.begin(), ds_names.end(), [&](std::string const& s)
            {
                bool b = (s == rs_get_device_name(dev, require_no_error()));
                if (b) std::cout << "Camera type " << s << std::endl;
                return b; }));
        }

        SECTION("device serial number has ten decimal digits")
        {
            const char * serial = rs_get_device_serial(dev, require_no_error());
            REQUIRE(strlen(serial) == 10);
            for (int i = 0; i<10; ++i) REQUIRE(isdigit(serial[i]));
        }
    }
}

TEST_CASE("DS-device devices support all required options", "[live] [DS-device]")
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

        SECTION("device supports standard picture options and DS-device extension options")
        {
            rs_set_device_option(dev, RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED, 1.0, require_no_error());

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
                RS_OPTION_FRAMES_QUEUE_SIZE
            };

            std::stringstream ss;
            for (int i = 0; i<RS_OPTION_COUNT; ++i)
            {
                ss.str(""); ss << "Verifying support for " << rs_option_to_string((rs_option)i);
                INFO(ss.str().c_str());
                if (std::find(std::begin(supported_options), std::end(supported_options), i) != std::end(supported_options))
                {
                    REQUIRE(rs_device_supports_option(dev, (rs_option)i, require_no_error()) == 1);
                }
                // There is no ELSE by design , as the test is intended to check only the common options support
            }
        }
    }
}

TEST_CASE("DS-device head content verification", "[live] [DS-device]")
{
    // Require at least one device to be plugged in
    safe_context ctx;
    const int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count > 0);

    std::vector<std::string> invalid_keywords = { "", "N/A" };
    // For each device
    for (int i = 0; i<device_count; ++i)
    {
        rs_device * dev = rs_get_device(ctx, 0, require_no_error());
        REQUIRE(dev != nullptr);

        SECTION("device supports standard picture options and DS-device extension options")
        {
            for (auto param = (int)rs_camera_info::RS_CAMERA_INFO_DEVICE_NAME; param < rs_camera_info::RS_CAMERA_INFO_COUNT; param++)
            {
                INFO("Testing " << rs_camera_info_to_string(rs_camera_info(param)));
                if (rs_supports_camera_info(dev,rs_camera_info(param), require_no_error()))
                {
                    auto val = rs_get_device_info(dev, rs_camera_info(param), require_no_error());
                    REQUIRE(std::none_of(invalid_keywords.begin(), invalid_keywords.end(), [&](std::string const& s) {return s == val; }));
                }
                else
                {
                    rs_error * e = nullptr;
                    REQUIRE(nullptr == rs_get_device_info(dev, rs_camera_info(param), &e));
                    REQUIRE(e != nullptr);
                }
            }
        }
    }
}

///////////////////////////////////
// Calibration information tests //
///////////////////////////////////

TEST_CASE("DS-device device extrinsics are within expected parameters", "[live] [DS-device]")
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

        SECTION("no extrinsic transformation between DEPTH and INFRARED")
        {
            rs_extrinsics extrin;
            rs_get_device_extrinsics(dev, RS_STREAM_DEPTH, RS_STREAM_INFRARED, &extrin, require_no_error());

            require_identity_matrix(extrin.rotation);
            require_zero_vector(extrin.translation);
        }

        SECTION("only x-axis translation (~70 mm) between DEPTH and INFRARED2")
        {
            rs_extrinsics extrin;
            rs_get_device_extrinsics(dev, RS_STREAM_DEPTH, RS_STREAM_INFRARED2, &extrin, require_no_error());

            require_identity_matrix(extrin.rotation);
            REQUIRE(extrin.translation[0] < -0.06f); // Some variation is allowed, but should report at least 60 mm in all cases
            REQUIRE(extrin.translation[0] > -0.08f); // Some variation is allowed, but should report at most 80 mm in all cases
            REQUIRE(extrin.translation[1] == 0.0f);
            REQUIRE(extrin.translation[2] == 0.0f);
        }

        SECTION("only translation between DEPTH and RECTIFIED_COLOR")
        {
            rs_extrinsics extrin;
            rs_get_device_extrinsics(dev, RS_STREAM_DEPTH, RS_STREAM_RECTIFIED_COLOR, &extrin, require_no_error());

            require_identity_matrix(extrin.rotation);
        }

        SECTION("depth scale is 0.001 (by default)")
        {
            REQUIRE(rs_get_device_depth_scale(dev, require_no_error()) == 0.001f);
        }
    }
}

TEST_CASE("DS-device infrared2 streaming modes exactly match infrared streaming modes", "[live] [DS-device]")
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

        // Require that there are a nonzero amount of infrared modes, and that infrared2 has the same number of modes
        const int infrared_mode_count = rs_get_stream_mode_count(dev, RS_STREAM_INFRARED, require_no_error());
        REQUIRE(infrared_mode_count > 0);
        REQUIRE(rs_get_stream_mode_count(dev, RS_STREAM_INFRARED2, require_no_error()) == infrared_mode_count);

        // For each streaming mode
        for (int j = 0; j<infrared_mode_count; ++j)
        {
            // Require that INFRARED and INFRARED2 streaming modes are exactly identical
            int infrared_width = 0, infrared_height = 0, infrared_framerate = 0; rs_format infrared_format = RS_FORMAT_ANY;
            rs_get_stream_mode(dev, RS_STREAM_INFRARED, j, &infrared_width, &infrared_height, &infrared_format, &infrared_framerate, require_no_error());

            int infrared2_width = 0, infrared2_height = 0, infrared2_framerate = 0; rs_format infrared2_format = RS_FORMAT_ANY;
            rs_get_stream_mode(dev, RS_STREAM_INFRARED2, j, &infrared2_width, &infrared2_height, &infrared2_format, &infrared2_framerate, require_no_error());

            REQUIRE(infrared_width == infrared2_width);
            REQUIRE(infrared_height == infrared2_height);
            REQUIRE(infrared_format == infrared2_format);
            REQUIRE(infrared_framerate == infrared2_framerate);

            // Require that the intrinsic for these streaming modes match exactly
            rs_enable_stream(dev, RS_STREAM_INFRARED, infrared_width, infrared_height, infrared_format, infrared_framerate, require_no_error());
            rs_enable_stream(dev, RS_STREAM_INFRARED2, infrared2_width, infrared2_height, infrared2_format, infrared2_framerate, require_no_error());

            REQUIRE(rs_get_stream_format(dev, RS_STREAM_INFRARED, require_no_error()) == rs_get_stream_format(dev, RS_STREAM_INFRARED2, require_no_error()));
            REQUIRE(rs_get_stream_framerate(dev, RS_STREAM_INFRARED, require_no_error()) == rs_get_stream_framerate(dev, RS_STREAM_INFRARED2, require_no_error()));

            rs_intrinsics infrared_intrin = {}, infrared2_intrin = {};
            rs_get_stream_intrinsics(dev, RS_STREAM_INFRARED, &infrared_intrin, require_no_error());
            rs_get_stream_intrinsics(dev, RS_STREAM_INFRARED2, &infrared2_intrin, require_no_error());
            REQUIRE(infrared_intrin.width == infrared_intrin.width);
            REQUIRE(infrared_intrin.height == infrared_intrin.height);
            REQUIRE(infrared_intrin.ppx == infrared_intrin.ppx);
            REQUIRE(infrared_intrin.ppy == infrared_intrin.ppy);
            REQUIRE(infrared_intrin.fx == infrared_intrin.fx);
            REQUIRE(infrared_intrin.fy == infrared_intrin.fy);
            REQUIRE(infrared_intrin.model == infrared_intrin.model);
            for (int k = 0; k<5; ++k) REQUIRE(infrared_intrin.coeffs[k] == infrared_intrin.coeffs[k]);
        }
    }
}

/////////////////////
// Streaming tests //
/////////////////////


///////////////////////////
// Depth streaming tests //
///////////////////////////

TEST_CASE("DS-device streams 480x360 depth", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60 } });
}

TEST_CASE("DS-device streams 628x468 depth", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 628, 468, RS_FORMAT_Z16, 60 } });
}

TEST_CASE("DS-device streams 320x240 depth", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 320, 240, RS_FORMAT_Z16, 60 } });
}

TEST_CASE("DS-device streams 480x360 depth (30 fps)", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 30 } });
}

TEST_CASE("DS-device streams 628x468 depth (30 fps)", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 628, 468, RS_FORMAT_Z16, 30 } });
}

TEST_CASE("DS-device streams 320x240 depth (30 fps)", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 320, 240, RS_FORMAT_Z16, 30 } });
}

TEST_CASE("DS-device streams 480x360 depth (90 fps)", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 90 } });
}

TEST_CASE("DS-device streams 628x468 depth (90 fps)", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 628, 468, RS_FORMAT_Z16, 90 } });
}

TEST_CASE("DS-device streams 320x240 depth (90 fps)", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 320, 240, RS_FORMAT_Z16, 90 } });
}

///////////////////////////
// Color streaming tests //
///////////////////////////

TEST_CASE("DS-device streams HD color", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_YUYV, 30 } });
}

TEST_CASE("DS-device streams VGA color", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 30 } });
}

TEST_CASE("DS-device streams VGA color (60 fps)", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 60 } });
}

TEST_CASE("DS-device streams 480x360 depth and HD color", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60 },
    { RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_YUYV, 30 } });
}

TEST_CASE("DS-device streams 628x468 depth and HD color", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 628, 468, RS_FORMAT_Z16, 60 },
    { RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_YUYV, 30 } });
}

TEST_CASE("DS-device streams 320x240 depth and HD color", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 320, 240, RS_FORMAT_Z16, 60 },
    { RS_STREAM_COLOR, 1920, 1080, RS_FORMAT_YUYV, 30 } });
}

TEST_CASE("DS-device streams 480x360 depth and VGA color", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60 },
    { RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 30 } });
}

TEST_CASE("DS-device streams 628x468 depth and VGA color", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 628, 468, RS_FORMAT_Z16, 60 },
    { RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 30 } });
}

TEST_CASE("DS-device streams 320x240 depth and VGA color", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 320, 240, RS_FORMAT_Z16, 60 },
    { RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 30 } });
}

TEST_CASE("DS-device streams 480x360 depth and VGA color (60 fps)", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60 },
    { RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 60 } });
}


//////////////////////////////
// Infrared streaming tests //
//////////////////////////////

TEST_CASE("DS-device streams 640x480 infrared (left 8 bit)", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8, 60 } });
}

TEST_CASE("DS-device streams 640x480 infrared (left 16 bit)", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 60 } });
}

TEST_CASE("DS-device streams 640x480 infrared (right 8 bit)", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_INFRARED2, 640, 480, RS_FORMAT_Y8, 60 } });
}

TEST_CASE("DS-device streams 640x480 infrared (right 16 bit)", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_INFRARED2, 640, 480, RS_FORMAT_Y16, 60 } });
}

TEST_CASE("DS-device streams 640x480 infrared (left+right 8 bit)", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8, 60 },
    { RS_STREAM_INFRARED2, 640, 480, RS_FORMAT_Y8, 60 } });
}

TEST_CASE("DS-device streams 640x480 infrared (left+right 16 bit)", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 60 },
    { RS_STREAM_INFRARED2, 640, 480, RS_FORMAT_Y16, 60 } });
}

TEST_CASE("DS-device streams 480x360 depth and 492x372 infrared (left+right 16 bit)", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60 },
    { RS_STREAM_INFRARED, 492, 372, RS_FORMAT_Y16, 60 },
    { RS_STREAM_INFRARED2, 492, 372, RS_FORMAT_Y16, 60 } });
}

TEST_CASE("DS-device streams 480x360 depth, VGA color, and 492x372 infrared", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60 },
    { RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 30 },
    { RS_STREAM_INFRARED, 492, 372, RS_FORMAT_Y16, 60 },
    { RS_STREAM_INFRARED2, 492, 372, RS_FORMAT_Y16, 60 } });
}

TEST_CASE("DS-device streams 628x468 depth, VGA color, and 640x480 infrared", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 628, 468, RS_FORMAT_Z16, 60 },
    { RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 30 },
    { RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y16, 60 },
    { RS_STREAM_INFRARED2, 640, 480, RS_FORMAT_Y16, 60 } });
}

TEST_CASE("DS-device streams 320x240 depth, VGA color, and 332x252 infrared", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 320, 240, RS_FORMAT_Z16, 60 },
    { RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 30 },
    { RS_STREAM_INFRARED, 332, 252, RS_FORMAT_Y16, 60 },
    { RS_STREAM_INFRARED2, 332, 252, RS_FORMAT_Y16, 60 } });
}

//////////////////////////////
// Cropped and padded modes //
//////////////////////////////

TEST_CASE("DS-device streams 640x480 depth and infrared", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60 },
    { RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8, 60 },
    { RS_STREAM_INFRARED2, 640, 480, RS_FORMAT_Y8, 60 } });
}

TEST_CASE("DS-device streams 628x468 depth and infrared", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 628, 468, RS_FORMAT_Z16, 60 },
    { RS_STREAM_INFRARED, 628, 468, RS_FORMAT_Y8, 60 },
    { RS_STREAM_INFRARED2, 628, 468, RS_FORMAT_Y8, 60 } });
}

TEST_CASE("DS-device streams 492x372 depth and infrared", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 492, 372, RS_FORMAT_Z16, 60 },
    { RS_STREAM_INFRARED, 492, 372, RS_FORMAT_Y8, 60 },
    { RS_STREAM_INFRARED2, 492, 372, RS_FORMAT_Y8, 60 } });
}

TEST_CASE("DS-device streams 480x360 depth and infrared", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60 },
    { RS_STREAM_INFRARED, 480, 360, RS_FORMAT_Y8, 60 },
    { RS_STREAM_INFRARED2, 480, 360, RS_FORMAT_Y8, 60 } });
}

TEST_CASE("DS-device streams 332x252 depth and infrared", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 332, 252, RS_FORMAT_Z16, 60 },
    { RS_STREAM_INFRARED, 332, 252, RS_FORMAT_Y8, 60 },
    { RS_STREAM_INFRARED2, 332, 252, RS_FORMAT_Y8, 60 } });
}

TEST_CASE("DS-device streams 320x240 depth and infrared", "[live] [DS-device] [one-camera]")
{
    test_ds_device_streaming({ { RS_STREAM_DEPTH, 320, 240, RS_FORMAT_Z16, 60 },
    { RS_STREAM_INFRARED, 320, 240, RS_FORMAT_Y8, 60 },
    { RS_STREAM_INFRARED2, 320, 240, RS_FORMAT_Y8, 60 } });
}

/////////////
// Options //
/////////////


TEST_CASE("DS-device supports RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED", "[live] [DS-device]")
{
    test_ds_device_option(RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED, { 0, 1 }, {}, BEFORE_START_DEVICE | AFTER_START_DEVICE);
}

TEST_CASE("DS-device RGB White Balance", "[live] [DS-device] [one-camera]")
{
                                                                              // Invalid values do not produce exception
    test_ds_device_option(RS_OPTION_COLOR_WHITE_BALANCE, { 2800, 3500, 4600, 5500, 6400 }, { /*100, 6510 */ }, BEFORE_START_DEVICE | AFTER_START_DEVICE);
}

TEST_CASE("DS-device supports RS_OPTION_R200_LR_GAIN", "[live] [DS-device]")
{
    test_ds_device_option(RS_OPTION_R200_LR_GAIN, { 100, 200, 400, 800, 1600 }, {}, BEFORE_START_DEVICE | AFTER_START_DEVICE); // Gain percentage   
}

TEST_CASE("DS-device supports RS_OPTION_R200_LR_EXPOSURE", "[live] [DS-device]")
{
    test_ds_device_option(RS_OPTION_R200_LR_EXPOSURE, { 40, 80, 160 }, {}, BEFORE_START_DEVICE | AFTER_START_DEVICE); // Tenths of milliseconds   
}

// Note: The DS-device firmware has some complications regarding emitter state before the device has been started
// The emitter will never be on if the device is not streaming, but the firmware will remember and respect any
// specified preferences for emitter enabled that are specified prior to streaming.

TEST_CASE("DS-device emitter defaults to off if depth is not enabled/streamed", "[live] [DS-device]")
{
    safe_context ctx;
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);

    REQUIRE(std::any_of(ds_names.begin(), ds_names.end(), [&](std::string const& s) {return s == rs_get_device_name(dev, require_no_error()); }));

    // Emitter enabled defaults to false
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);

    // Enabling non-depth streams does not change the emitter's state
    rs_enable_stream_preset(dev, RS_STREAM_COLOR, RS_PRESET_BEST_QUALITY, require_no_error());
    rs_enable_stream_preset(dev, RS_STREAM_INFRARED, RS_PRESET_BEST_QUALITY, require_no_error());
    rs_enable_stream_preset(dev, RS_STREAM_INFRARED2, RS_PRESET_BEST_QUALITY, require_no_error());
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);

    // Starting the device does not change the emitter's state
    rs_start_device(dev, require_no_error());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);
}

TEST_CASE("DS-device emitter defaults to on if depth is enabled/streamed", "[live] [DS-device]")
{
    safe_context ctx;
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(std::any_of(ds_names.begin(), ds_names.end(), [&](std::string const& s) {return s == rs_get_device_name(dev, require_no_error()); }));

    // Emitter enabled defaults to false
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);

    // Enabling depth stream causes the emitter to be enabled
    rs_enable_stream_preset(dev, RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY, require_no_error());
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 1);

    // Starting the device does not change the emitter's state
    rs_start_device(dev, require_no_error());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 1);
}

TEST_CASE("DS-device emitter can be enabled even if depth is not enabled/streamed", "[live] [DS-device]")
{
    safe_context ctx;
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(std::any_of(ds_names.begin(), ds_names.end(), [&](std::string const& s) {return s == rs_get_device_name(dev, require_no_error()); }));

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
    rs_start_device(dev, require_no_error());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 1);
}

TEST_CASE("DS-device emitter can be turned off even if depth is enabled/streamed", "[live] [DS-device]")
{
    safe_context ctx;
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(std::any_of(ds_names.begin(), ds_names.end(), [&](std::string const& s) {return s == rs_get_device_name(dev, require_no_error()); }));

    // Emitter enabled defaults to false
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);

    // Enabling depth stream causes the emitter to be enabled
    rs_enable_stream_preset(dev, RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY, require_no_error());
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 1);

    // The emitter can be turned off even though depth is streamed
    rs_set_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, 0, require_no_error());
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);

    // Starting the device does not change the emitter's state
    rs_start_device(dev, require_no_error());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);
}

TEST_CASE("DS-device emitter can be turned on and off after streaming has begun", "[live] [DS-device]")
{
    safe_context ctx;
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(std::any_of(ds_names.begin(), ds_names.end(), [&](std::string const& s) {return s == rs_get_device_name(dev, require_no_error()); }));

    // The emitter defaults to on when depth is streamed
    rs_enable_stream_preset(dev, RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY, require_no_error());
    rs_start_device(dev, require_no_error());
    std::this_thread::sleep_for(std::chrono::seconds(1));
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 1);

    // The emitter can be turned off
    rs_set_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, 0, require_no_error());
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 0);

    // The emitter can be turned back on
    rs_set_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, 1, require_no_error());
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 1);
}

TEST_CASE("DS-device supports RS_OPTION_R200_DEPTH_UNITS", "[live] [DS-device]")
{
    safe_context ctx;
    REQUIRE(rs_get_device_count(ctx, require_no_error()) == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);
    REQUIRE(std::any_of(ds_names.begin(), ds_names.end(), [&](std::string const& s) {return s == rs_get_device_name(dev, require_no_error()); }));

    // By default, depth unit is 1000 micrometers (1 mm)
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_DEPTH_UNITS, require_no_error()) == 1000);
    REQUIRE(rs_get_device_depth_scale(dev, require_no_error()) == 0.001f);

    for (int value : {100, 500, 1000, 2000, 10000})
    {
        // Set depth units (specified in micrometers) and verify that depth scale (specified in meters) changes appropriately
        rs_set_device_option(dev, RS_OPTION_R200_DEPTH_UNITS, value, require_no_error());
        REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_DEPTH_UNITS, require_no_error()) == value);
        REQUIRE(rs_get_device_depth_scale(dev, require_no_error()) == (float)value / 1000000);
    }
}

TEST_CASE("DS-device supports RS_OPTION_R200_DEPTH_CLAMP_MIN", "[live] [DS-device]")
{
    test_ds_device_option(RS_OPTION_R200_DEPTH_CLAMP_MIN, { 0, 500, 1000, 2000 }, {}, BEFORE_START_DEVICE);
}

TEST_CASE("DS-device supports RS_OPTION_R200_DEPTH_CLAMP_MAX", "[live] [DS-device]")
{
    test_ds_device_option(RS_OPTION_R200_DEPTH_CLAMP_MAX, { 500, 1000, 2000, USHRT_MAX }, {}, BEFORE_START_DEVICE);
}

TEST_CASE("DS-device verify standard UVC Controls set/get", "[live] [DS-device]")
{
    // Require only one device to be plugged in
    safe_context ctx;
    const int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count == 1);

    // For each device
    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);

    REQUIRE(std::any_of(ds_names.begin(), ds_names.end(), [&](std::string const& s) {return s == rs_get_device_name(dev, require_no_error()); }));

    // Enabling non-depth streams does not change the emitter's state
    rs_enable_stream_preset(dev, RS_STREAM_COLOR, RS_PRESET_BEST_QUALITY, require_no_error());
    rs_enable_stream_preset(dev, RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY, require_no_error());

    rs_set_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, 1, require_no_error());
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 1);
    // Starting the device does not change the emitter's state
    rs_start_device(dev, require_no_error());

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    REQUIRE(rs_get_device_option(dev, RS_OPTION_R200_EMITTER_ENABLED, require_no_error()) == 1);

    rs_option first = RS_OPTION_COLOR_BACKLIGHT_COMPENSATION;
    rs_option last = RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE;

    std::vector<rs_option> test_options;
    std::vector<double> initial_values;
    std::vector<double> modified_values;
    std::vector<double> verification_values;
    for (int i=first; i<= last; i++)
        test_options.push_back((rs_option)i);

    initial_values.resize(test_options.size());
    modified_values.resize(test_options.size());
    verification_values.resize(test_options.size());

    rs_get_device_options(dev,test_options.data(),(unsigned int)test_options.size(),initial_values.data(), require_no_error());

    //for (size_t i=first; i<= last; i++)
    //    std::cout << "Option " << rs_option_to_string((rs_option)i) << " : initial value " << initial_values[i] <<std::endl;

    double min=0, max=0, step=0;
    for (int i=first; i<= last; i++)
    {
        rs_get_device_option_range(dev,(rs_option)i,&min,&max,&step,require_no_error());
        if (initial_values[i] == max)
            modified_values[i] = initial_values[i]- step;
        else
            modified_values[i] = initial_values[i]+ step;
    }

    // Apply all properties with the modified values
    rs_set_device_options(dev,test_options.data(),(unsigned int)test_options.size(),modified_values.data(), require_no_error());
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // Verify
    rs_get_device_options(dev,test_options.data(),(unsigned int)test_options.size(),verification_values.data(), require_no_error());

    //for (size_t i=first; i<= last; i++)
    //    std::cout << "Option " << rs_option_to_string((rs_option)i) << " Requested value = " << modified_values[i] << " Actual value = " << verification_values[i] << std::endl;

    for (int i=first; i<= last; i++)
    {
#if defined(_WINDOWS) || defined(WIN32) || defined(WIN64)
        if (((rs_option)i == rs_option::RS_OPTION_COLOR_EXPOSURE || (rs_option)i == rs_option::RS_OPTION_COLOR_WHITE_BALANCE))
            continue;
#endif

        REQUIRE(modified_values[i]!=initial_values[i]);
        REQUIRE(modified_values[i]==verification_values[i]);
    }

    rs_stop_device(dev, require_no_error());
}

//////////////////////////////////////////
// Stop, reconfigure, and restart tests //
//////////////////////////////////////////

TEST_CASE("a single DS-device can stream a variety of reasonable streaming mode combinations", "[live] [DS-device] [one-camera]")
{
    safe_context ctx;

    SECTION("exactly one device is connected")
    {
        int device_count = rs_get_device_count(ctx, require_no_error());
        REQUIRE(device_count == 1);
    }

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);

    SECTION("device name identification ")
    {
        REQUIRE(std::any_of(ds_names.begin(), ds_names.end(), [&](std::string const& s)
        {
            bool b = (s == rs_get_device_name(dev, require_no_error()));
            if (b) std::cout << "Camera type " << s << std::endl;
            return b; }));
    }

    SECTION("streaming is possible in some reasonable configurations")
    {
        SECTION("streaming DEPTH 480, 360, RS_FORMAT_Z16, 60")
        {
            test_streaming(dev, {
                { RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60 }
            });
        }

        SECTION("streaming [DEPTH,480,360,Z16,60] [COLOR,480,360,RGB8,60]")
        {
            test_streaming(dev, {
                { RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60 },
                { RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60 }
            });
        }

        SECTION("streaming [DEPTH,480,360,Z16,60] [IR,480,360,Y8,60]")
        {
            test_streaming(dev, {
                { RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60 },
                { RS_STREAM_INFRARED, 480, 360, RS_FORMAT_Y8, 60 }
            });
        }

        SECTION("streaming [IR,492,372,Y16,60] [IR2,492,372,Y16,60]")
        {
            test_streaming(dev, {
                { RS_STREAM_INFRARED, 492, 372, RS_FORMAT_Y16, 60 },
                { RS_STREAM_INFRARED2, 492, 372, RS_FORMAT_Y16, 60 }
            });
        }

        SECTION("streaming [DEPTH,480,360,Z16,60] [COLOR,640,480,RGB8,60] [IR,480,360,Y8,60] [IR2,480,360,Y8,60]")
        {
            test_streaming(dev, {
                { RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60 },
                { RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60 },
                { RS_STREAM_INFRARED, 480, 360, RS_FORMAT_Y8, 60 },
                { RS_STREAM_INFRARED2, 480, 360, RS_FORMAT_Y8, 60 }
            });
        }
    }
}

TEST_CASE("streaming five configurations sequentionally", "[live] [DS-device] [one-camera]")
{
    safe_context ctx;

    SECTION("exactly one device is connected")
    {
        int device_count = rs_get_device_count(ctx, require_no_error());
        REQUIRE(device_count == 1);
    }

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);

    REQUIRE(std::any_of(ds_names.begin(), ds_names.end(), [&](std::string const& s)
    {
        bool b = (s == rs_get_device_name(dev, require_no_error()));
        if (b) std::cout << "Camera type " << s << std::endl;
        return b; }));

    test_streaming(dev, {
        { RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60 }
    });

    test_streaming(dev, {
        { RS_STREAM_COLOR, 640, 480, RS_FORMAT_YUYV, 60 }
    });

    test_streaming(dev, {
        { RS_STREAM_INFRARED, 492, 372, RS_FORMAT_Y16, 60 },
        { RS_STREAM_INFRARED2, 492, 372, RS_FORMAT_Y16, 60 }
    });

    test_streaming(dev, {
        { RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60 },
        { RS_STREAM_INFRARED, 480, 360, RS_FORMAT_Y8, 60 }
    });

    test_streaming(dev, {
        { RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60 },
        { RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60 },
        { RS_STREAM_INFRARED, 480, 360, RS_FORMAT_Y8, 60 },
        { RS_STREAM_INFRARED2, 480, 360, RS_FORMAT_Y8, 60 }
    });
}

#endif /* !defined(MAKEFILE) || ( defined(LR200_TEST) || defined(R200_TEST) || defined(ZR300_TEST) ) */
