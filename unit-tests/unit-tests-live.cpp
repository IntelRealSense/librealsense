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
    rs::context ctx;
    std::vector<rs::device> list;
    REQUIRE_NOTHROW(list = ctx.query_devices());
    const int device_count = list.size();
    REQUIRE(device_count > 0);

    // For each device
    for(int i=0; i<device_count; ++i)
    {
        rs::device dev = list[i];
        //REQUIRE(dev != nullptr);

        SECTION( "supported device metadata strings are nonempty, unsupported ones throw" )
        {
            for (int i = 0; i < RS_CAMERA_INFO_COUNT; ++i) {
                bool is_supported;
                REQUIRE_NOTHROW(is_supported = dev.supports(rs_camera_info(i)));
                if (is_supported) REQUIRE(dev.get_camera_info(rs_camera_info(i)) != nullptr);
                else REQUIRE_THROWS_AS(dev.get_camera_info(rs_camera_info(i)), rs::error);
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
    rs::context ctx;
    std::vector<rs::device> list;
    REQUIRE_NOTHROW(list = ctx.query_devices());
    const int device_count = list.size();
    REQUIRE(device_count > 0);

    rs::util::config config;
    REQUIRE_NOTHROW(config.enable_all(rs::preset::best_quality));

    // For each device
    for (int i = 0; i < device_count; ++i)
    {
        rs::device dev = list[i];
        //REQUIRE(dev != nullptr);

        // Configure all supported streams to run at 30 frames per second
        auto streams = config.open(dev);

        for (int i = 0; i < 5; i++)
        {
            // Test sequence
            REQUIRE_NOTHROW(streams.start([](rs_frame * fref) {}));
            REQUIRE_NOTHROW(streams.stop());
        }

        config.disable_all();
    }
}

///////////////////////////////////
// Calibration information tests //
///////////////////////////////////

TEST_CASE( "no extrinsic transformation between a stream and itself", "[live]" )
{
    // Require at least one device to be plugged in
    rs::context ctx;
    std::vector<rs::device> list;
    REQUIRE_NOTHROW(list = ctx.query_devices());
    const int device_count = list.size();
    REQUIRE(device_count > 0);

    // For each device
    for(int i=0; i<device_count; ++i)
    {
        rs::device dev = list[i];
        //REQUIRE(dev != nullptr);

        for(int j=0; j<RS_SUBDEVICE_COUNT; ++j)
        {
            if (!dev.supports(rs_subdevice(j))) continue;

            rs_extrinsics extrin;
            try {
                extrin = dev.get_extrinsics(rs_subdevice(j), rs_subdevice(j));
            }
            catch (rs::error &e) {
                // if device isn't calibrated, get_extrinsics must error out (according to old comment. Might not be true under new API)
                WARN(e.what());
                continue;
            }
            
            require_identity_matrix(extrin.rotation);
            require_zero_vector(extrin.translation);
        }
    }
}

TEST_CASE( "extrinsic transformation between two streams is a rigid transform", "[live]" )
{
    // Require at least one device to be plugged in
    rs::context ctx;
    std::vector<rs::device> list;
    REQUIRE_NOTHROW(list = ctx.query_devices());
    const int device_count = list.size();
    REQUIRE(device_count > 0);

    // For each device
    for(int i=0; i<device_count; ++i)
    {
        rs::device dev = list[i];
        //REQUIRE(dev != nullptr);

        // For every pair of streams
        for(int j=0; j<RS_SUBDEVICE_COUNT; ++j)
        {
            if (!dev.supports(rs_subdevice(j))) continue;

            for(int k=j+1; k<RS_SUBDEVICE_COUNT; ++k)
            {
                if (!dev.supports(rs_subdevice(k))) continue;
            
                // Extrinsics from A to B should have an orthonormal 3x3 rotation matrix and a translation vector of magnitude less than 10cm
                rs_extrinsics a_to_b;

                try {
                    a_to_b = dev.get_extrinsics(rs_subdevice(j), rs_subdevice(k));
                }
                catch (rs::error &e) {
                    WARN(e.what());
                    continue;
                }

                require_rotation_matrix(a_to_b.rotation);
                REQUIRE( vector_length(a_to_b.translation) < 0.1f );

                // Extrinsics from B to A should be the inverse of extrinsics from A to B
                rs_extrinsics b_to_a;
                REQUIRE_NOTHROW(b_to_a = dev.get_extrinsics(rs_subdevice(k), rs_subdevice(j)));

                require_transposed(a_to_b.rotation, b_to_a.rotation);
                REQUIRE( b_to_a.rotation[0] * a_to_b.translation[0] + b_to_a.rotation[3] * a_to_b.translation[1] + b_to_a.rotation[6] * a_to_b.translation[2] == Approx(-b_to_a.translation[0]) );
                REQUIRE( b_to_a.rotation[1] * a_to_b.translation[0] + b_to_a.rotation[4] * a_to_b.translation[1] + b_to_a.rotation[7] * a_to_b.translation[2] == Approx(-b_to_a.translation[1]) );
                REQUIRE( b_to_a.rotation[2] * a_to_b.translation[0] + b_to_a.rotation[5] * a_to_b.translation[1] + b_to_a.rotation[8] * a_to_b.translation[2] == Approx(-b_to_a.translation[2]) );
            }
        }
    }
}

TEST_CASE( "extrinsic transformations are transitive", "[live]" )
{
    // Require at least one device to be plugged in
    rs::context ctx;
    std::vector<rs::device> list;
    REQUIRE_NOTHROW(list = ctx.query_devices());
    const int device_count = list.size();
    REQUIRE(device_count > 0);

    // For each device
    for(int i=0; i<device_count; ++i)
    {
        rs::device dev = list[i];
        //REQUIRE(dev != nullptr);

        // For every set of subdevices
        for(int a=0; a<RS_SUBDEVICE_COUNT; ++a)
        {
            if (!dev.supports(rs_subdevice(a))) continue;
            
            for(int b=0; b<RS_SUBDEVICE_COUNT; ++b)
            {
                if (!dev.supports(rs_subdevice(b))) continue;
                
                for(int c=0; c<RS_SUBDEVICE_COUNT; ++c)
                {
                    if (!dev.supports(rs_subdevice(c))) continue;

                    // Require that the composition of a_to_b and b_to_c is equal to a_to_c
                    rs_extrinsics a_to_b, b_to_c, a_to_c;
                    
                    try {
                        a_to_b = dev.get_extrinsics(rs_subdevice(a), rs_subdevice(b));
                        b_to_c = dev.get_extrinsics(rs_subdevice(b), rs_subdevice(c));
                        a_to_c = dev.get_extrinsics(rs_subdevice(a), rs_subdevice(c));
                    }
                    catch (rs::error &e)
                    {
                        WARN(e.what());
                        continue;
                    }

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

// break up
TEST_CASE( "streaming modes sanity check", "[live]" )
{
    // Require at least one device to be plugged in
    rs::context ctx;
    std::vector<rs::device> list;
    REQUIRE_NOTHROW(list = ctx.query_devices());
    const int device_count = list.size();
    REQUIRE(device_count > 0);

    // For each device
    for(int i=0; i<device_count; ++i)
    {
        rs::device dev = list[i];
        //REQUIRE(dev != nullptr);

        // for each subdevice on the device
        for (auto && subdevice : dev) {
            // make sure they provide at least one streaming mode
            std::vector<rs::stream_profile> stream_profiles;
            REQUIRE_NOTHROW(stream_profiles = subdevice.get_stream_modes());
            REQUIRE(stream_profiles.size() > 0);

            // for each stream profile provided:
            for (auto profile : stream_profiles) {
                SECTION("check stream profile settings are sane") {
                    // require that the settings are sane
                    REQUIRE(profile.width >= 320);
                    REQUIRE(profile.width <= 1920);
                    REQUIRE(profile.height >= 180);
                    REQUIRE(profile.height <= 1080);
                    REQUIRE(profile.format > RS_FORMAT_ANY);
                    REQUIRE(profile.format < RS_FORMAT_COUNT);
                    REQUIRE(profile.fps >= 2);
                    REQUIRE(profile.fps <= 300);

                    // require that we can start streaming this mode
                    auto lock = subdevice.open({ profile }); // TODO: add default constructor to rs::streaming_lock for more coverage
                    // TODO: make callback confirm stream format/dimensions/framerate
                    REQUIRE_NOTHROW(lock.start([](rs_frame * fref) {}));

                    // Require that we can disable the stream afterwards
                    REQUIRE_NOTHROW(lock.stop());
                }
                SECTION("check stream intrinsics are sane") {
                    rs_intrinsics intrin;
                    REQUIRE_NOTHROW(intrin = subdevice.get_intrinsics(profile));

                    // Intrinsic width/height must match width/height of streaming mode we requested
                    REQUIRE(intrin.width == profile.width);
                    REQUIRE(intrin.height == profile.height);

                    // Principal point must be within center 20% of image
                    REQUIRE(intrin.ppx > profile.width * 0.4f);
                    REQUIRE(intrin.ppx < profile.width * 0.6f);
                    REQUIRE(intrin.ppy > profile.height * 0.4f);
                    REQUIRE(intrin.ppy < profile.height * 0.6f);

                    // Focal length must be nonnegative (todo - Refine requirements based on known expected FOV)
                    REQUIRE(intrin.fx > 0.0f);
                    REQUIRE(intrin.fy > 0.0f);
                }
            }
        }
    }
}

#endif /* !defined(MAKEFILE) || ( defined(LIVE_TEST) ) */
