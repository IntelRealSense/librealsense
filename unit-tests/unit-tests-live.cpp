// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This set of tests is valid for any number and combination of RealSense cameras, including R200 and F200 //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if !defined(MAKEFILE) || ( defined(LIVE_TEST) )

#include "unit-tests-common.h"

TEST_CASE( "Device metadata enumerates correctly", "[live]" )
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

        SECTION( "device metadata strings are nonempty" )
        {
            REQUIRE( rs_get_device_name(dev, require_no_error()) != nullptr );
            REQUIRE( rs_get_device_serial(dev, require_no_error()) != nullptr );
            REQUIRE( rs_get_device_firmware_version(dev, require_no_error()) != nullptr );
        }

        SECTION( "device supports standard picture options" )
        {
            for(int i=RS_OPTION_COLOR_BACKLIGHT_COMPENSATION; i<=RS_OPTION_COLOR_WHITE_BALANCE; ++i)
            {
                REQUIRE(rs_device_supports_option(dev, (rs_option)i, require_no_error()) == 1);
            }
        }
    }
}

////////////////////////////////////////////////////////
// Test basic streaming functionality //
////////////////////////////////////////////////////////
TEST_CASE("Start-Stop stream sequence", "[live]")
{
    // Require at least one device to be plugged in
    safe_context ctx;
    const int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count > 0);

    std::vector<rs_stream> supported_streams;

    // For each device
    for(int i=0; i<device_count; ++i)
    {
        rs_device * dev = rs_get_device(ctx, 0, require_no_error());
        REQUIRE(dev != nullptr);

        for (int i = (int)rs_capabilities::RS_CAPABILITIES_DEPTH; i <= (int)rs_capabilities::RS_CAPABILITIES_FISH_EYE; i++)
            if (rs_supports(dev,(rs_capabilities)i, require_no_error()))
                supported_streams.push_back((rs_stream)i);

        // Configure all supported streams to run at 30 frames per second
        for (auto & stream : supported_streams)
            rs_enable_stream_preset(dev, stream, rs_preset::RS_PRESET_BEST_QUALITY, require_no_error());

        for (int i = 0; i< 5; i++)
        {
            // Test sequence
            rs_start_device(dev, require_no_error());
            rs_stop_device(dev, require_no_error());
        }

        for (auto & stream : supported_streams)
            rs_disable_stream(dev,stream,require_no_error());
    }
}

///////////////////////////////////
// Calibration information tests //
///////////////////////////////////

TEST_CASE( "no extrinsic transformation between a stream and itself", "[live]" )
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

        for(int j=0; j<RS_STREAM_COUNT; ++j)
        {
            rs_extrinsics extrin = {};
            rs_error * e = nullptr;
            rs_get_device_extrinsics(dev, (rs_stream)j, (rs_stream)j, &extrin, &e);
            if (!e) // if device is not calibrated, rs_get_device_extrinsic has to return an error
            {
                require_identity_matrix(extrin.rotation);
                require_zero_vector(extrin.translation);
            }
        }
    }
}

TEST_CASE( "extrinsic transformation between two streams is a rigid transform", "[live]" )
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

            for(int k=j+1; k<RS_STREAM_COUNT; ++k)
            {
                const rs_stream stream_b = (rs_stream)k;
            
                // Extrinsics from A to B should have an orthonormal 3x3 rotation matrix and a translation vector of magnitude less than 10cm
                rs_extrinsics a_to_b = {};

                rs_error * e = nullptr;

                rs_get_device_extrinsics(dev, stream_a, stream_b, &a_to_b, &e);

                if (!e)
                {
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
        }
    }
}

TEST_CASE( "extrinsic transformations are transitive", "[live]" )
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
        for(int a=0; a<RS_STREAM_COUNT; ++a)
        {
            for(int b=0; b<RS_STREAM_COUNT; ++b)
            {
                for(int c=0; c<RS_STREAM_COUNT; ++c)
                {
                    // Require that the composition of a_to_b and b_to_c is equal to a_to_c
                    rs_extrinsics a_to_b, b_to_c, a_to_c;
                    rs_error * a_to_b_e = nullptr,* a_to_c_e = nullptr, * b_to_c_e = nullptr;

                    rs_get_device_extrinsics(dev, (rs_stream)a, (rs_stream)b, &a_to_b, &a_to_b_e);
                    rs_get_device_extrinsics(dev, (rs_stream)b, (rs_stream)c, &b_to_c, &b_to_c_e);
                    rs_get_device_extrinsics(dev, (rs_stream)a, (rs_stream)c, &a_to_c, &a_to_c_e);

                    if ((!a_to_b_e) && (!b_to_c_e) && (!a_to_c_e))
                    {
                        // a_to_c.rotation == a_to_b.rotation * b_to_c.rotation
                        REQUIRE( a_to_c.rotation[0] == Approx(a_to_b.rotation[0] * b_to_c.rotation[0] + a_to_b.rotation[3] * b_to_c.rotation[1] + a_to_b.rotation[6] * b_to_c.rotation[2]) );
                        REQUIRE( a_to_c.rotation[2] == Approx(a_to_b.rotation[2] * b_to_c.rotation[0] + a_to_b.rotation[5] * b_to_c.rotation[1] + a_to_b.rotation[8] * b_to_c.rotation[2]) );
                        REQUIRE( a_to_c.rotation[1] == Approx(a_to_b.rotation[1] * b_to_c.rotation[0] + a_to_b.rotation[4] * b_to_c.rotation[1] + a_to_b.rotation[7] * b_to_c.rotation[2]) );
                        REQUIRE( a_to_c.rotation[3] == Approx(a_to_b.rotation[0] * b_to_c.rotation[3] + a_to_b.rotation[3] * b_to_c.rotation[4] + a_to_b.rotation[6] * b_to_c.rotation[5]) );
                        REQUIRE( a_to_c.rotation[4] == Approx(a_to_b.rotation[1] * b_to_c.rotation[3] + a_to_b.rotation[4] * b_to_c.rotation[4] + a_to_b.rotation[7] * b_to_c.rotation[5]) );
                        REQUIRE( a_to_c.rotation[5] == Approx(a_to_b.rotation[2] * b_to_c.rotation[3] + a_to_b.rotation[5] * b_to_c.rotation[4] + a_to_b.rotation[8] * b_to_c.rotation[5]) );
                        REQUIRE( a_to_c.rotation[6] == Approx(a_to_b.rotation[0] * b_to_c.rotation[6] + a_to_b.rotation[3] * b_to_c.rotation[7] + a_to_b.rotation[6] * b_to_c.rotation[8]) );
                        REQUIRE( a_to_c.rotation[7] == Approx(a_to_b.rotation[1] * b_to_c.rotation[6] + a_to_b.rotation[4] * b_to_c.rotation[7] + a_to_b.rotation[7] * b_to_c.rotation[8]) );
                        REQUIRE( a_to_c.rotation[8] == Approx(a_to_b.rotation[2] * b_to_c.rotation[6] + a_to_b.rotation[5] * b_to_c.rotation[7] + a_to_b.rotation[8] * b_to_c.rotation[8]) );

                        // a_to_c.translation = a_to_b.transform(b_to_c.translation)
                        REQUIRE( a_to_c.translation[0] == Approx(a_to_b.rotation[0] * b_to_c.translation[0] + a_to_b.rotation[3] * b_to_c.translation[1] + a_to_b.rotation[6] * b_to_c.translation[2] + a_to_b.translation[0]) );
                        REQUIRE( a_to_c.translation[1] == Approx(a_to_b.rotation[1] * b_to_c.translation[0] + a_to_b.rotation[4] * b_to_c.translation[1] + a_to_b.rotation[7] * b_to_c.translation[2] + a_to_b.translation[1]) );
                        REQUIRE( a_to_c.translation[2] == Approx(a_to_b.rotation[2] * b_to_c.translation[0] + a_to_b.rotation[5] * b_to_c.translation[1] + a_to_b.rotation[8] * b_to_c.translation[2] + a_to_b.translation[2]) );
                   }
                }
            }
        }
    }
}

TEST_CASE( "aligned images have no extrinsic transformation from the image they are aligned to", "[live]" )
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
                REQUIRE( height >= 180 );
                REQUIRE( height <= 1080 );
                REQUIRE( format > RS_FORMAT_ANY );
                REQUIRE( format < RS_FORMAT_COUNT );
                REQUIRE( framerate >= 2 );
                REQUIRE( framerate <= 300 );

                // Require that we can set the stream to this mode
                rs_enable_stream(dev, stream, width, height, format, framerate, require_no_error());
                REQUIRE(rs_is_stream_enabled(dev, stream, require_no_error()) == 1);
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

                // Focal length must be nonnegative (todo - Refine requirements based on known expected FOV)
                REQUIRE( intrin.fx > 0.0f );
                REQUIRE( intrin.fy > 0.0f );

                // Require that we can disable the stream afterwards
                rs_disable_stream(dev, stream, require_no_error());
                REQUIRE(rs_is_stream_enabled(dev, stream, require_no_error()) == 0);
            }

            // Require that we cannot retrieve intrinsic/format/framerate when stream is disabled
            REQUIRE(rs_is_stream_enabled(dev, stream, require_no_error()) == 0);
            rs_intrinsics intrin;
            rs_get_stream_intrinsics(dev, stream, &intrin, require_error(std::string("stream not enabled: ") + rs_stream_to_string(stream)));
            rs_get_stream_format(dev, stream, require_error(std::string("stream not enabled: ") + rs_stream_to_string(stream)));
            rs_get_stream_framerate(dev, stream, require_error(std::string("stream not enabled: ") + rs_stream_to_string(stream)));
        }
    }
}

//TEST_CASE( "synthetic streaming mode properties are correct", "[live]" )
//{
//    // Require at least one device to be plugged in
//    safe_context ctx;
//    const int device_count = rs_get_device_count(ctx, require_no_error());
//    REQUIRE(device_count > 0);
//
//    // For each device
//    for(int i=0; i<device_count; ++i)
//    {
//        rs_device * dev = rs_get_device(ctx, 0, require_no_error());    
//        REQUIRE(dev != nullptr);
//
//        // For each combination of COLOR and DEPTH streaming modes
//        const int color_mode_count = rs_get_stream_mode_count(dev, RS_STREAM_COLOR, require_no_error());
//        const int depth_mode_count = rs_get_stream_mode_count(dev, RS_STREAM_DEPTH, require_no_error());
//        for(int j=0; j<color_mode_count; ++j)
//        {
//            // Enable a COLOR mode and retrieve intrinsics
//            int color_width = 0, color_height = 0, color_framerate = 0; rs_format color_format = RS_FORMAT_ANY;
//            rs_get_stream_mode(dev, RS_STREAM_COLOR, j, &color_width, &color_height, &color_format, &color_framerate, require_no_error());
//            rs_enable_stream(dev, RS_STREAM_COLOR, color_width, color_height, color_format, color_framerate, require_no_error());
//
//            rs_intrinsics color_intrin = {};
//            rs_get_stream_intrinsics(dev, RS_STREAM_COLOR, &color_intrin, require_no_error());
//
//            // Validate that RECTIFIED_COLOR properties match size/format/framerate of COLOR, and have no distortion
//            REQUIRE( rs_get_stream_format(dev, RS_STREAM_RECTIFIED_COLOR, require_no_error()) == color_format );
//            REQUIRE( rs_get_stream_framerate(dev, RS_STREAM_RECTIFIED_COLOR, require_no_error()) == color_framerate );
//
//            rs_intrinsics rectified_color_intrin = {};
//            rs_get_stream_intrinsics(dev, RS_STREAM_RECTIFIED_COLOR, &rectified_color_intrin, require_no_error());
//            REQUIRE( rectified_color_intrin.width  == color_width  );
//            REQUIRE( rectified_color_intrin.height == color_height );
//            REQUIRE( rectified_color_intrin.ppx > color_width * 0.4f );
//            REQUIRE( rectified_color_intrin.ppx < color_width * 0.6f );
//            REQUIRE( rectified_color_intrin.ppy > color_height * 0.4f );
//            REQUIRE( rectified_color_intrin.ppy < color_height * 0.6f );
//            REQUIRE( rectified_color_intrin.fx > 0.0f );
//            REQUIRE( rectified_color_intrin.fy > 0.0f );
//            REQUIRE( rectified_color_intrin.model == RS_DISTORTION_NONE );
//            for(int k=0; k<5; ++k) REQUIRE( rectified_color_intrin.coeffs[k] == 0.0f );
//
//            for(int k=0; k<depth_mode_count; ++k)
//            {
//                // Enable a DEPTH mode and retrieve intrinsics
//                int depth_width = 0, depth_height = 0, depth_framerate = 0; rs_format depth_format = RS_FORMAT_ANY; 
//                rs_get_stream_mode(dev, RS_STREAM_DEPTH, k, &depth_width, &depth_height, &depth_format, &depth_framerate, require_no_error());
//                rs_enable_stream(dev, RS_STREAM_DEPTH, depth_width, depth_height, depth_format, color_framerate, require_no_error());
//
//                rs_intrinsics depth_intrin = {};
//                rs_get_stream_intrinsics(dev, RS_STREAM_DEPTH, &depth_intrin, require_no_error());
//
//                // COLOR_ALIGNED_TO_DEPTH must have same format/framerate as COLOR
//                REQUIRE( rs_get_stream_format(dev, RS_STREAM_COLOR_ALIGNED_TO_DEPTH, require_no_error()) == color_format );
//                REQUIRE( rs_get_stream_framerate(dev, RS_STREAM_COLOR_ALIGNED_TO_DEPTH, require_no_error()) == color_framerate );
//
//                // COLOR_ALIGNED_TO_DEPTH must have same intrinsics as DEPTH
//                rs_intrinsics color_aligned_to_depth_intrin = {};
//                rs_get_stream_intrinsics(dev, RS_STREAM_COLOR_ALIGNED_TO_DEPTH, &color_aligned_to_depth_intrin, require_no_error());
//                REQUIRE( color_aligned_to_depth_intrin.width  == depth_intrin.width  );
//                REQUIRE( color_aligned_to_depth_intrin.height == depth_intrin.height );
//                REQUIRE( color_aligned_to_depth_intrin.ppx    == depth_intrin.ppx    );
//                REQUIRE( color_aligned_to_depth_intrin.ppy    == depth_intrin.ppy    );
//                REQUIRE( color_aligned_to_depth_intrin.fx     == depth_intrin.fx     );
//                REQUIRE( color_aligned_to_depth_intrin.fy     == depth_intrin.fy     );
//                REQUIRE( color_aligned_to_depth_intrin.model  == depth_intrin.model  );
//                for(int l=0; l<5; ++l) REQUIRE( color_aligned_to_depth_intrin.coeffs[l]  == depth_intrin.coeffs[l] );
//
//                // DEPTH_ALIGNED_TO_COLOR must have same format/framerate as DEPTH
//                REQUIRE( rs_get_stream_format(dev, RS_STREAM_DEPTH_ALIGNED_TO_COLOR, require_no_error()) == depth_format );
//                REQUIRE( rs_get_stream_framerate(dev, RS_STREAM_DEPTH_ALIGNED_TO_COLOR, require_no_error()) == color_framerate );
//
//                // DEPTH_ALIGNED_TO_COLOR must have same intrinsics as COLOR
//                rs_intrinsics depth_aligned_to_color_intrin = {};
//                rs_get_stream_intrinsics(dev, RS_STREAM_DEPTH_ALIGNED_TO_COLOR, &depth_aligned_to_color_intrin, require_no_error());
//                REQUIRE( depth_aligned_to_color_intrin.width  == color_intrin.width  );
//                REQUIRE( depth_aligned_to_color_intrin.height == color_intrin.height );
//                REQUIRE( depth_aligned_to_color_intrin.ppx    == color_intrin.ppx    );
//                REQUIRE( depth_aligned_to_color_intrin.ppy    == color_intrin.ppy    );
//                REQUIRE( depth_aligned_to_color_intrin.fx     == color_intrin.fx     );
//                REQUIRE( depth_aligned_to_color_intrin.fy     == color_intrin.fy     );
//                REQUIRE( depth_aligned_to_color_intrin.model  == color_intrin.model  );
//                for(int l=0; l<5; ++l) REQUIRE( depth_aligned_to_color_intrin.coeffs[l]  == color_intrin.coeffs[l] );
//
//                // DEPTH_ALIGNED_TO_RECTIFIED_COLOR must have same format/framerate as DEPTH
//                REQUIRE( rs_get_stream_format(dev, RS_STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR, require_no_error()) == depth_format );
//                REQUIRE( rs_get_stream_framerate(dev, RS_STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR, require_no_error()) == color_framerate );
//
//                // DEPTH_ALIGNED_TO_RECTIFIED_COLOR must have same intrinsics as RECTIFIED_COLOR
//                rs_intrinsics depth_aligned_to_rectified_color_intrin = {};
//                rs_get_stream_intrinsics(dev, RS_STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR, &depth_aligned_to_rectified_color_intrin, require_no_error());
//                REQUIRE( depth_aligned_to_rectified_color_intrin.width  == rectified_color_intrin.width  );
//                REQUIRE( depth_aligned_to_rectified_color_intrin.height == rectified_color_intrin.height );
//                REQUIRE( depth_aligned_to_rectified_color_intrin.ppx    == rectified_color_intrin.ppx    );
//                REQUIRE( depth_aligned_to_rectified_color_intrin.ppy    == rectified_color_intrin.ppy    );
//                REQUIRE( depth_aligned_to_rectified_color_intrin.fx     == rectified_color_intrin.fx     );
//                REQUIRE( depth_aligned_to_rectified_color_intrin.fy     == rectified_color_intrin.fy     );
//                REQUIRE( depth_aligned_to_rectified_color_intrin.model  == rectified_color_intrin.model  );
//                for(int l=0; l<5; ++l) REQUIRE( depth_aligned_to_rectified_color_intrin.coeffs[l]  == rectified_color_intrin.coeffs[l] );
//            }
//        }
//    }
//}

#endif /* !defined(MAKEFILE) || ( defined(LIVE_TEST) ) */
