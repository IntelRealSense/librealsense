/////////////////////////////////////////////////////////
// This set of tests is valid only for the F200 camera //
/////////////////////////////////////////////////////////

#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include "unit-tests-common.h"

#include <sstream>

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

        SECTION( "device serial number has ten decimal digits" )
        {
            const char * serial = rs_get_device_serial(dev, require_no_error());
            REQUIRE(strlen(serial) == 18);
            for(int i=0; i<10; ++i) REQUIRE(isxdigit(serial[i]));
        }

        SECTION( "device supports standard picture options and F200 extension options, and nothing else" )
        {
            for(int i=0; i<RS_OPTION_COUNT; ++i)
            {
                if(i >= RS_OPTION_COLOR_BACKLIGHT_COMPENSATION && i <= RS_OPTION_COLOR_WHITE_BALANCE)
                {
                    REQUIRE(rs_device_supports_option(dev, (rs_option)i, require_no_error()) == 1);
                }
                else if(i >= RS_OPTION_F200_LASER_POWER && i <= RS_OPTION_F200_CONFIDENCE_THRESHOLD)
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

        SECTION( "only translation between DEPTH and RECTIFIED_COLOR" )
        {
            rs_extrinsics extrin;
            rs_get_device_extrinsics(dev, RS_STREAM_DEPTH, RS_STREAM_RECTIFIED_COLOR, &extrin, require_no_error());

            require_identity_matrix(extrin.rotation);
        }

        SECTION( "depth scale is 1/32 mm" )
        {
            REQUIRE( rs_get_device_depth_scale(dev, require_no_error()) == Approx(1.0f/32000) );
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
        test_streaming(dev, {
            {RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60}
        });

        test_streaming(dev, {
            {RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60},
            {RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60}
        });

        test_streaming(dev, {
            {RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60},
            {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8, 60}
        });

        test_streaming(dev, {
            {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8, 60},
        });

        test_streaming(dev, {
            {RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60},
            {RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60},
            {RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8, 60}
        });
    }
}