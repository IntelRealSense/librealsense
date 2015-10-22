#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include "test-common.h"

#include <sstream>

TEST_CASE( "a single DS4 enumerates correctly", "[live] [ds4] [one-camera]" )
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

    SECTION( "device serial number has ten decimal digits" )
    {
        const char * serial = rs_get_device_serial(dev, require_no_error());
        REQUIRE(strlen(serial) == 10);
        for(int i=0; i<10; ++i) REQUIRE(isdigit(serial[i]));
    }

    SECTION( "device firmware version is a nonempty string" )
    {
        const char * version = rs_get_device_firmware_version(dev, require_no_error());
        REQUIRE(strlen(version) > 0);
    }

    SECTION( "device supports standard picture options and R200 extension options, and nothing else" )
    {
        for(int i=0; i<RS_OPTION_COUNT; ++i)
        {
            if(i >= RS_OPTION_COLOR_BACKLIGHT_COMPENSATION && i <= RS_OPTION_COLOR_WHITE_BALANCE)
            {
                REQUIRE(rs_device_supports_option(dev, (rs_option)i, require_no_error()) == 1);
            }
            else if(i >= RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED && i <= RS_OPTION_R200_DISPARITY_SHIFT)
            {
                REQUIRE(rs_device_supports_option(dev, (rs_option)i, require_no_error()) == 1);
            }
            else
            {
                REQUIRE(rs_device_supports_option(dev, (rs_option)i, require_no_error()) == 0);
            }
        }

        // Require the option requests with indices outside of [0,RS_OPTION_COUNT) indicate an error
        rs_device_supports_option(dev, (rs_option)-1, require_error("bad enum value for argument \"option\""));
        rs_device_supports_option(dev, RS_OPTION_COUNT, require_error("bad enum value for argument \"option\""));
    }
}

///////////////////////////////////
// Calibration information tests //
///////////////////////////////////

TEST_CASE( "device extrinsics are sane", "[live]" )
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

        // For every pair of streams
        for(int j=0; j<RS_STREAM_COUNT; ++j)
        {
            const rs_stream stream_a = (rs_stream)j;

            // Extrinsics from A to A should be an identity transform
            rs_extrinsics a_to_a = {};
            rs_get_device_extrinsics(dev, stream_a, stream_a, &a_to_a, require_no_error());
            require_identity_matrix(a_to_a.rotation);
            require_zero_vector(a_to_a.translation);

            for(int k=j+1; k<RS_STREAM_COUNT; ++k)
            {
                const rs_stream stream_b = (rs_stream)k;
            
                // Extrinsics from A to B should have an orthonormal 3x3 rotation matrix and a translation vector of magnitude less than 10cm
                rs_extrinsics a_to_b = {};
                rs_get_device_extrinsics(dev, stream_a, stream_b, &a_to_b, require_no_error());
                require_rotation_matrix(a_to_b.rotation);
                REQUIRE( vector_length(a_to_b.translation) < 0.1f );

                // Extrinsics from B to A should be the inverse of extrinsics from A to B
                rs_extrinsics b_to_a = {};
                rs_get_device_extrinsics(dev, stream_b, stream_a, &b_to_a, require_no_error());
                require_transposed(a_to_b.rotation, b_to_a.rotation);
                REQUIRE( b_to_a.rotation[0] * a_to_b.translation[0] + b_to_a.rotation[3] * a_to_b.translation[1] + b_to_a.rotation[6] * a_to_b.translation[2] == Approx(-b_to_a.translation[0]) );
                REQUIRE( b_to_a.rotation[1] * a_to_b.translation[0] + b_to_a.rotation[4] * a_to_b.translation[1] + b_to_a.rotation[7] * a_to_b.translation[2] == Approx(-b_to_a.translation[1]) );
                REQUIRE( b_to_a.rotation[2] * a_to_b.translation[0] + b_to_a.rotation[5] * a_to_b.translation[1] + b_to_a.rotation[8] * a_to_b.translation[2] == Approx(-b_to_a.translation[2]) );
            }
        }

        // Require no TRANSLATION (but rotation is acceptable) between RECTIFIED_COLOR and COLOR (because they come from the same imager)
        rs_extrinsics extrin = {};
        rs_get_device_extrinsics(dev, RS_STREAM_RECTIFIED_COLOR, RS_STREAM_COLOR, &extrin, require_no_error());
        require_zero_vector(extrin.translation);

        // Require no extrinsic transformation between COLOR_ALIGNED_TO_DEPTH and DEPTH (because, by definition, they are aligned)
        rs_get_device_extrinsics(dev, RS_STREAM_COLOR_ALIGNED_TO_DEPTH, RS_STREAM_DEPTH, &extrin, require_no_error());
        require_identity_matrix(extrin.rotation);
        require_zero_vector(extrin.translation);

        // Require no extrinsic transformation between DEPTH_ALIGNED_TO_COLOR and COLOR (because, by definition, they are aligned)
        rs_get_device_extrinsics(dev, RS_STREAM_DEPTH_ALIGNED_TO_COLOR, RS_STREAM_COLOR, &extrin, require_no_error());
        require_identity_matrix(extrin.rotation);
        require_zero_vector(extrin.translation);

        // Require no extrinsic transformation between DEPTH_ALIGNED_TO_RECTIFIED_COLOR and RECTIFIED_COLOR (because, by definition, they are aligned)
        rs_get_device_extrinsics(dev, RS_STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR, RS_STREAM_RECTIFIED_COLOR, &extrin, require_no_error());
        require_identity_matrix(extrin.rotation);
        require_zero_vector(extrin.translation);
    }
}

TEST_CASE( "DS4 device extrinsics are within expected parameters", "[live] [ds4]" )
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

TEST_CASE( "streaming mode intrinsics are sane", "[live]" )
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

        // For each of the basic streams
        for(auto stream : {RS_STREAM_DEPTH, RS_STREAM_COLOR, RS_STREAM_INFRARED})
        {
            // Require that there are modes for this stream
            const int stream_mode_count = rs_get_stream_mode_count(dev, stream, require_no_error());
            REQUIRE(stream_mode_count > 0);

            // For each streaming mode
            for(int j=0; j<stream_mode_count; ++j)
            {
                // Retrieve mode settings
                int width = 0, height = 0, framerate = 0;
                rs_format format = RS_FORMAT_ANY;
                rs_get_stream_mode(dev, stream, j, &width, &height, &format, &framerate, require_no_error());

                // Require that the mode settings are sane
                REQUIRE( width >= 320 );
                REQUIRE( width <= 1920 );
                REQUIRE( height >= 240 );
                REQUIRE( height <= 1080 );
                REQUIRE( format > RS_FORMAT_ANY );
                REQUIRE( format < RS_FORMAT_COUNT );
                REQUIRE( framerate >= 15 );
                REQUIRE( framerate <= 90 );

                // Require that we can set the stream to this mode
                rs_enable_stream(dev, stream, width, height, format, framerate, require_no_error());
                REQUIRE(rs_stream_is_enabled(dev, stream, require_no_error()) == 1);
                REQUIRE(rs_get_stream_format(dev, stream, require_no_error()) == format);
                REQUIRE(rs_get_stream_framerate(dev, stream, require_no_error()) == framerate);

                // Intrinsic width/height must match width/height of streaming mode we requested
                rs_intrinsics intrin;
                rs_get_stream_intrinsics(dev, stream, &intrin, require_no_error());
                REQUIRE( intrin.width == width );
                REQUIRE( intrin.height == height );

                // Principal point must be within center 20% of image
                REQUIRE( intrin.ppx > width * 0.4f );
                REQUIRE( intrin.ppx < width * 0.6f );
                REQUIRE( intrin.ppy > height * 0.4f );
                REQUIRE( intrin.ppy < height * 0.6f );

                // Focal length must be nonnegative (TODO: Refine requirements based on known expected FOV)
                REQUIRE( intrin.fx > 0.0f );
                REQUIRE( intrin.fy > 0.0f );

                // Require that we can disable the stream afterwards
                rs_disable_stream(dev, stream, require_no_error());
                REQUIRE(rs_stream_is_enabled(dev, stream, require_no_error()) == 0);
            }

            // Require that we cannot retrieve intrinsics/format/framerate when stream is disabled
            REQUIRE(rs_stream_is_enabled(dev, stream, require_no_error()) == 0);
            rs_intrinsics intrin;
            rs_get_stream_intrinsics(dev, stream, &intrin, require_error(std::string("stream not enabled: ") + rs_stream_to_string(stream)));
            rs_get_stream_format(dev, stream, require_error(std::string("stream not enabled: ") + rs_stream_to_string(stream)));
            rs_get_stream_framerate(dev, stream, require_error(std::string("stream not enabled: ") + rs_stream_to_string(stream)));
        }
    }
}

TEST_CASE( "DS4 infrared2 streaming modes exactly match infrared streaming modes", "[live] [ds4]" )
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

TEST_CASE( "synthetic streaming mode properties are correct", "[live]" )
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

        // For each combination of COLOR and DEPTH streaming modes
        const int color_mode_count = rs_get_stream_mode_count(dev, RS_STREAM_COLOR, require_no_error());
        const int depth_mode_count = rs_get_stream_mode_count(dev, RS_STREAM_DEPTH, require_no_error());
        for(int j=0; j<color_mode_count; ++j)
        {
            // Enable a COLOR mode and retrieve intrinsics
            int color_width = 0, color_height = 0, color_framerate = 0; rs_format color_format = RS_FORMAT_ANY;
            rs_get_stream_mode(dev, RS_STREAM_COLOR, j, &color_width, &color_height, &color_format, &color_framerate, require_no_error());
            rs_enable_stream(dev, RS_STREAM_COLOR, color_width, color_height, color_format, color_framerate, require_no_error());

            rs_intrinsics color_intrin = {};
            rs_get_stream_intrinsics(dev, RS_STREAM_COLOR, &color_intrin, require_no_error());

            // Validate that RECTIFIED_COLOR properties match size/format/framerate of COLOR, and have no distortion
            REQUIRE( rs_get_stream_format(dev, RS_STREAM_RECTIFIED_COLOR, require_no_error()) == color_format );
            REQUIRE( rs_get_stream_framerate(dev, RS_STREAM_RECTIFIED_COLOR, require_no_error()) == color_framerate );

            rs_intrinsics rectified_color_intrin = {};
            rs_get_stream_intrinsics(dev, RS_STREAM_RECTIFIED_COLOR, &rectified_color_intrin, require_no_error());
            REQUIRE( rectified_color_intrin.width  == color_width  );
            REQUIRE( rectified_color_intrin.height == color_height );
            REQUIRE( rectified_color_intrin.ppx > color_width * 0.4f );
            REQUIRE( rectified_color_intrin.ppx < color_width * 0.6f );
            REQUIRE( rectified_color_intrin.ppy > color_height * 0.4f );
            REQUIRE( rectified_color_intrin.ppy < color_height * 0.6f );
            REQUIRE( rectified_color_intrin.fx > 0.0f );
            REQUIRE( rectified_color_intrin.fy > 0.0f );
            REQUIRE( rectified_color_intrin.model == RS_DISTORTION_NONE );
            for(int k=0; k<5; ++k) REQUIRE( rectified_color_intrin.coeffs[k] == 0.0f );

            for(int k=0; k<depth_mode_count; ++k)
            {
                // Enable a DEPTH mode and retrieve intrinsics
                int depth_width = 0, depth_height = 0, depth_framerate = 0; rs_format depth_format = RS_FORMAT_ANY; 
                rs_get_stream_mode(dev, RS_STREAM_DEPTH, k, &depth_width, &depth_height, &depth_format, &depth_framerate, require_no_error());
                rs_enable_stream(dev, RS_STREAM_DEPTH, depth_width, depth_height, depth_format, depth_framerate, require_no_error());

                rs_intrinsics depth_intrin = {};
                rs_get_stream_intrinsics(dev, RS_STREAM_DEPTH, &depth_intrin, require_no_error());

                // COLOR_ALIGNED_TO_DEPTH must have same format/framerate as COLOR
                REQUIRE( rs_get_stream_format(dev, RS_STREAM_COLOR_ALIGNED_TO_DEPTH, require_no_error()) == color_format );
                REQUIRE( rs_get_stream_framerate(dev, RS_STREAM_COLOR_ALIGNED_TO_DEPTH, require_no_error()) == color_framerate );

                // COLOR_ALIGNED_TO_DEPTH must have same intrinsics as DEPTH
                rs_intrinsics color_aligned_to_depth_intrin = {};
                rs_get_stream_intrinsics(dev, RS_STREAM_COLOR_ALIGNED_TO_DEPTH, &color_aligned_to_depth_intrin, require_no_error());
                REQUIRE( color_aligned_to_depth_intrin.width  == depth_intrin.width  );
                REQUIRE( color_aligned_to_depth_intrin.height == depth_intrin.height );
                REQUIRE( color_aligned_to_depth_intrin.ppx    == depth_intrin.ppx    );
                REQUIRE( color_aligned_to_depth_intrin.ppy    == depth_intrin.ppy    );
                REQUIRE( color_aligned_to_depth_intrin.fx     == depth_intrin.fx     );
                REQUIRE( color_aligned_to_depth_intrin.fy     == depth_intrin.fy     );
                REQUIRE( color_aligned_to_depth_intrin.model  == depth_intrin.model  );
                for(int l=0; l<5; ++l) REQUIRE( color_aligned_to_depth_intrin.coeffs[l]  == depth_intrin.coeffs[l] );

                // DEPTH_ALIGNED_TO_COLOR must have same format/framerate as DEPTH
                REQUIRE( rs_get_stream_format(dev, RS_STREAM_DEPTH_ALIGNED_TO_COLOR, require_no_error()) == depth_format );
                REQUIRE( rs_get_stream_framerate(dev, RS_STREAM_DEPTH_ALIGNED_TO_COLOR, require_no_error()) == depth_framerate );

                // DEPTH_ALIGNED_TO_COLOR must have same intrinsics as COLOR
                rs_intrinsics depth_aligned_to_color_intrin = {};
                rs_get_stream_intrinsics(dev, RS_STREAM_DEPTH_ALIGNED_TO_COLOR, &depth_aligned_to_color_intrin, require_no_error());
                REQUIRE( depth_aligned_to_color_intrin.width  == color_intrin.width  );
                REQUIRE( depth_aligned_to_color_intrin.height == color_intrin.height );
                REQUIRE( depth_aligned_to_color_intrin.ppx    == color_intrin.ppx    );
                REQUIRE( depth_aligned_to_color_intrin.ppy    == color_intrin.ppy    );
                REQUIRE( depth_aligned_to_color_intrin.fx     == color_intrin.fx     );
                REQUIRE( depth_aligned_to_color_intrin.fy     == color_intrin.fy     );
                REQUIRE( depth_aligned_to_color_intrin.model  == color_intrin.model  );
                for(int l=0; l<5; ++l) REQUIRE( depth_aligned_to_color_intrin.coeffs[l]  == color_intrin.coeffs[l] );

                // DEPTH_ALIGNED_TO_RECTIFIED_COLOR must have same format/framerate as DEPTH
                REQUIRE( rs_get_stream_format(dev, RS_STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR, require_no_error()) == depth_format );
                REQUIRE( rs_get_stream_framerate(dev, RS_STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR, require_no_error()) == depth_framerate );

                // DEPTH_ALIGNED_TO_RECTIFIED_COLOR must have same intrinsics as RECTIFIED_COLOR
                rs_intrinsics depth_aligned_to_rectified_color_intrin = {};
                rs_get_stream_intrinsics(dev, RS_STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR, &depth_aligned_to_rectified_color_intrin, require_no_error());
                REQUIRE( depth_aligned_to_rectified_color_intrin.width  == rectified_color_intrin.width  );
                REQUIRE( depth_aligned_to_rectified_color_intrin.height == rectified_color_intrin.height );
                REQUIRE( depth_aligned_to_rectified_color_intrin.ppx    == rectified_color_intrin.ppx    );
                REQUIRE( depth_aligned_to_rectified_color_intrin.ppy    == rectified_color_intrin.ppy    );
                REQUIRE( depth_aligned_to_rectified_color_intrin.fx     == rectified_color_intrin.fx     );
                REQUIRE( depth_aligned_to_rectified_color_intrin.fy     == rectified_color_intrin.fy     );
                REQUIRE( depth_aligned_to_rectified_color_intrin.model  == rectified_color_intrin.model  );
                for(int l=0; l<5; ++l) REQUIRE( depth_aligned_to_rectified_color_intrin.coeffs[l]  == rectified_color_intrin.coeffs[l] );
            }
        }
    }
}

/////////////////////
// Streaming tests //
/////////////////////

struct stream_mode { rs_stream stream; int width, height; rs_format format; int framerate; };

void test_streaming(rs_device * device, std::initializer_list<stream_mode> modes)
{
    std::ostringstream ss;
    for(auto & mode : modes)
    {
        ss << rs_stream_to_string(mode.stream) << "=" << mode.width << "x" << mode.height << " " << rs_format_to_string(mode.format) << "@" << mode.framerate << "Hz ";
    }

    SECTION( "stream " + ss.str() )
    {
        for(auto & mode : modes)
        {
            rs_enable_stream(device, mode.stream, mode.width, mode.height, mode.format, mode.framerate, require_no_error());
            REQUIRE( rs_stream_is_enabled(device, mode.stream, require_no_error()) == 1 );
        }
        rs_start_device(device, require_no_error());
        REQUIRE( rs_device_is_streaming(device, require_no_error()) == 1 );

        rs_wait_for_frames(device, require_no_error());
        for(auto & mode : modes)
        {
            REQUIRE( rs_stream_is_enabled(device, mode.stream, require_no_error()) == 1 );
            REQUIRE( rs_get_frame_data(device, mode.stream, require_no_error()) != nullptr );
            REQUIRE( rs_get_frame_timestamp(device, mode.stream, require_no_error()) >= 0 );
        }

        for(int i=0; i<100; ++i)
        {
            rs_wait_for_frames(device, require_no_error());
        }
        for(auto & mode : modes)
        {
            REQUIRE( rs_stream_is_enabled(device, mode.stream, require_no_error()) == 1 );
            REQUIRE( rs_get_frame_data(device, mode.stream, require_no_error()) != nullptr );
            REQUIRE( rs_get_frame_timestamp(device, mode.stream, require_no_error()) >= 0 );
        }

        rs_stop_device(device, require_no_error());
        REQUIRE( rs_device_is_streaming(device, require_no_error()) == 0 );
        for(auto & mode : modes)
        {
            rs_disable_stream(device, mode.stream, require_no_error());
            REQUIRE( rs_stream_is_enabled(device, mode.stream, require_no_error()) == 0 );
        }
    }
}

TEST_CASE( "a single DS4 can stream a variety of reasonable streaming mode combinations", "[live] [ds4] [one-camera]" )
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
        test_streaming(dev, {
            {RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60}
        });

        test_streaming(dev, {
            {RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60},
            {RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60}
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
            {RS_STREAM_DEPTH, 480, 360, RS_FORMAT_Z16, 60},
            {RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60},
            {RS_STREAM_INFRARED, 480, 360, RS_FORMAT_Y8, 60},
            {RS_STREAM_INFRARED2, 480, 360, RS_FORMAT_Y8, 60}
        });
    }
}