// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This set of tests is valid for any number and combination of RealSense cameras, including R200 and F200 //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "unit-tests-common.h"

TEST_CASE( "Device metadata enumerates correctly", "[live]" )
{
    auto ctx = make_context(__FUNCTION__);
    // Require at least one device to be plugged in
    std::vector<rs::device> list;
    REQUIRE_NOTHROW(list = ctx.query_devices());
    const int device_count = list.size();
    REQUIRE(device_count > 0);

    // For each device
    for(int i=0; i<device_count; ++i)
    {
        rs::device dev = list[i];

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
    auto ctx = make_context(__FUNCTION__);
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
    auto ctx = make_context(__FUNCTION__);
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
    auto ctx = make_context(__FUNCTION__);
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
    auto ctx = make_context(__FUNCTION__);
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
    auto ctx = make_context(__FUNCTION__);
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
                    rs::streaming_lock lock;
                    REQUIRE_NOTHROW(lock = subdevice.open({ profile }));
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

TEST_CASE("check option API", "[live][options]") 
{
    // Require at least one device to be plugged in
    auto ctx = make_context(__FUNCTION__);
    std::vector<rs::device> list;
    REQUIRE_NOTHROW(list = ctx.query_devices());
    REQUIRE(list.size() > 0);

    // for each device
    for (auto & dev : list) {
        // for each subdevice
        for (auto && subdevice : dev) {
            // for each option
            for (int i = 0; i < RS_OPTION_COUNT; ++i) {
                rs_option opt = rs_option(i);
                bool is_opt_supported;
                REQUIRE_NOTHROW(is_opt_supported = subdevice.supports(opt));
                

                SECTION("Ranges are sane") {
                    if (!is_opt_supported) {
                        REQUIRE_THROWS_AS(subdevice.get_option_range(opt), rs::error);
                    }
                    else
                    {
                        rs::option_range range;
                        REQUIRE_NOTHROW(range = subdevice.get_option_range(opt));

                        // a couple sanity checks
                        REQUIRE(range.min < range.max);
                        REQUIRE(range.min + range.step <= range.max);
                        REQUIRE(range.step > 0);
                        REQUIRE(range.def <= range.max);
                        REQUIRE(range.min <= range.def);

                        // TODO: check that range.def == range.min + k*range.step for some k?
                        // TODO: some sort of bounds checking against constants?
                    }
                }
                SECTION("get_option returns a legal value") {
                    if (!is_opt_supported) {
                        REQUIRE_THROWS_AS(subdevice.get_option(opt), rs::error);
                    }
                    else
                    {
                        auto range = subdevice.get_option_range(opt);
                        float value;
                        REQUIRE_NOTHROW(value = subdevice.get_option(opt));

                        // value in range. Do I need to account for epsilon in lt[e]/gt[e] comparisons?
                        REQUIRE(value >= range.min);
                        REQUIRE(value <= range.max);

                        // value doesn't change between two gets (if no additional threads are calling set)
                        REQUIRE(subdevice.get_option(opt) == Approx(value));

                        // REQUIRE(value == Approx(range.def)); // Not sure if this is a reasonable check
                        // TODO: make sure value == range.min + k*range.step for some k?
                    }
                }
                SECTION("set opt doesn't like bad values") {
                    if (!is_opt_supported) {
                        REQUIRE_THROWS_AS(subdevice.set_option(opt, 1), rs::error);
                    }
                    else
                    {
                        auto range = subdevice.get_option_range(opt);

                        // minimum should work, as should maximum
                        REQUIRE_NOTHROW(subdevice.set_option(opt, range.min));
                        REQUIRE_NOTHROW(subdevice.set_option(opt, range.max));

                        int n_steps = (range.max - range.min) / range.step;
                        
                        // check a few arbitrary points along the scale
                        REQUIRE_NOTHROW(subdevice.set_option(opt, range.min + (1 % n_steps)*range.step));
                        REQUIRE_NOTHROW(subdevice.set_option(opt, range.min + (11 % n_steps)*range.step));
                        REQUIRE_NOTHROW(subdevice.set_option(opt, range.min + (111 % n_steps)*range.step));
                        REQUIRE_NOTHROW(subdevice.set_option(opt, range.min + (1111 % n_steps)*range.step));

                        // below min and above max shouldn't work
                        REQUIRE_THROWS_AS(subdevice.set_option(opt, range.min - range.step), rs::error);
                        REQUIRE_THROWS_AS(subdevice.set_option(opt, range.max + range.step), rs::error);

                        // make sure requesting value in the range, but not a legal step doesn't work
                        // TODO: maybe something for range.step < 1 ?
                        for (int i = 1; i < range.step; i++) {
                            CAPTURE(range.step);
                            CAPTURE(i);
                            REQUIRE_THROWS_AS(subdevice.set_option(opt, range.min + i), rs::error);
                        }
                    }
                }
                SECTION("check get/set sequencing works as expected") {
                    if (!is_opt_supported) continue;
                    auto range = subdevice.get_option_range(opt);
                    
                    // setting a valid value lets you get that value back
                    subdevice.set_option(opt, range.min);
                    REQUIRE(subdevice.get_option(opt) == Approx(range.min));

                    // setting an invalid value returns the last set valid value.
                    REQUIRE_THROWS(subdevice.set_option(opt, range.max + range.step));
                    REQUIRE(subdevice.get_option(opt) == Approx(range.min));

                    subdevice.set_option(opt, range.max);
                    REQUIRE_THROWS(subdevice.set_option(opt, range.min - range.step));
                    REQUIRE(subdevice.get_option(opt) == Approx(range.max));
             
                }
                SECTION("get_description returns a non-empty, non-null string") {
                    if (!is_opt_supported) {
                        REQUIRE_THROWS_AS(subdevice.get_option_description(opt), rs::error);
                    }
                    else
                    {
                        REQUIRE(subdevice.get_option_description(opt) != nullptr);
                        REQUIRE(std::string(subdevice.get_option_description(opt)) != std::string(""));
                    }
                }
                // TODO: tests for get_option_value_description? possibly too open a function to have useful tests
            }
        }
    }
}

TEST_CASE("a single subdevice can only be opened once, different subdevices can be opened simultaneously", "[live][multicam]") 
{
    // Require at least one device to be plugged in
    auto ctx = make_context(__FUNCTION__);
    std::vector<rs::device> list;
    REQUIRE_NOTHROW(list = ctx.query_devices());
    REQUIRE(list.size() > 0);
    SECTION("Single context") {
        SECTION("subdevices on a single device") 
        {
            for (auto & dev : list) 
            {
                SECTION("opening the same subdevice multiple times") 
                {
                    for (auto && subdevice : dev) 
                    {
                        auto modes = subdevice.get_stream_modes();
                        REQUIRE(modes.size() > 0);
                        rs::streaming_lock lock1, lock2;
                        CAPTURE(rs_subdevice(subdevice));
                        CAPTURE(modes[0].stream);
                        REQUIRE_NOTHROW(lock1 = subdevice.open( modes[0] ));
                        SECTION("same mode") {
                            // selected, but not streaming
                            REQUIRE_THROWS_AS(lock2 = subdevice.open({ modes[0] }), rs::error);

                            // streaming
                            REQUIRE_NOTHROW(lock1.start([](rs_frame * fref) {}));
                            REQUIRE_THROWS_AS(lock2 = subdevice.open({ modes[0] }), rs::error);
                        }
                        SECTION("different modes") {
                            if (modes.size() == 1) {
                                WARN("device " << dev.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME) << " S/N: " << dev.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER) << " w/ FW v" << dev.get_camera_info(RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION) << ":");
                                WARN("subdevice " << rs_subdevice(subdevice) << " has only 1 supported streaming mode. Skipping Same Subdevice, different modes test.");
                            } 
                            else
                            {
                                // selected, but not streaming
                                REQUIRE_THROWS_AS(lock2 = subdevice.open({ modes[1] }), rs::error);

                                // streaming
                                REQUIRE_NOTHROW(lock1.start([](rs_frame * fref) {}));
                                REQUIRE_THROWS_AS(lock2 = subdevice.open({ modes[1] }), rs::error);
                            }
                        }
                        REQUIRE_NOTHROW(lock1.stop());
                    }
                }
                SECTION("opening different subdevices") {
                    for (auto & subdevice1 : dev) {
                        for (auto & subdevice2 : dev) {
                            if (subdevice1 == subdevice2) continue;

                            rs::streaming_lock lock1;
                            CAPTURE(rs_subdevice(subdevice1));
                            CAPTURE(rs_subdevice(subdevice2));

                            // get first lock
                            REQUIRE_NOTHROW(lock1 = subdevice1.open(subdevice1.get_stream_modes()[0]));
                            
                            // selected, but not streaming
                            {
                                rs::streaming_lock lock2;
                                CAPTURE(subdevice2.get_stream_modes()[0].stream);
                                REQUIRE_NOTHROW(lock2 = subdevice2.open(subdevice2.get_stream_modes()[0]));
                                REQUIRE_NOTHROW(lock2.start([](rs_frame * fref) {}));
                                REQUIRE_NOTHROW(lock2.stop());
                            }

                            // streaming
                            {
                                rs::streaming_lock lock2;
                                REQUIRE_NOTHROW(lock1.start([](rs_frame * fref) {}));
                                REQUIRE_NOTHROW(lock2 = subdevice2.open(subdevice2.get_stream_modes()[0]));
                                REQUIRE_NOTHROW(lock2.start([](rs_frame * fref) {}));
                                // stop streaming in opposite order just to be sure that works too
                                REQUIRE_NOTHROW(lock1.stop());
                                REQUIRE_NOTHROW(lock2.stop());
                            }
                        }
                    }
                }
            }
        }
        SECTION("multiple devices") {
            if (list.size() == 1) {
                WARN("Only one device connected. Skipping multi-device test");
            }
            else
            {
                for (auto & dev1 : list) {
                    for (auto & dev2 : list) {
                        // couldn't think of a better way to compare the two...
                        if (dev1.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER)
                            == dev2.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER)
                            )
                            continue;
                        rs::util::config config1, config2;
                        config1.enable_all(rs::preset::best_quality);
                        config2.enable_all(rs::preset::best_quality);

                        rs::util::multistream streams1, streams2;
                        REQUIRE_NOTHROW(streams1 = config1.open(dev1));
                        REQUIRE_NOTHROW(streams2 = config2.open(dev2));

                        REQUIRE_NOTHROW(streams1.start([](rs_frame * fref) {}));
                        REQUIRE_NOTHROW(streams2.start([](rs_frame * fref) {}));
                        REQUIRE_NOTHROW(streams1.stop());
                        REQUIRE_NOTHROW(streams2.stop());
                    }
                }
            }
        }
    }
    SECTION("two contexts") 
    {
        auto ctx2 = make_context("two_contexts");
        std::vector<rs::device> list2;
        REQUIRE_NOTHROW(list2 = ctx2.query_devices());
        REQUIRE(list2.size() == list.size());
        SECTION("subdevices on a single device") {
            for (auto & dev1 : list) {
                for (auto & dev2 : list2) {
                    if (dev1.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER)
                        != dev2.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER)
                        )
                        continue;
                    for (auto && sub1 : dev1) {
                        for (auto && sub2 : dev2) {
                            if (sub1 == sub2) {
                                SECTION("same subdevice") {
                                    // get modes
                                    std::vector<rs::stream_profile> modes1, modes2;
                                    REQUIRE_NOTHROW(modes1 = sub1.get_stream_modes());
                                    REQUIRE_NOTHROW(modes2 = sub2.get_stream_modes());
                                    REQUIRE(modes1.size() > 0);
                                    REQUIRE(modes1.size() == modes2.size());
                                    // require that the lists are the same (disregarding order)
                                    for (auto profile : modes1) {
                                        REQUIRE(std::any_of(begin(modes2), end(modes2), [&](const rs::stream_profile & p)
                                        {
                                            return profile.format == p.format && profile.fps == p.fps
                                                && profile.height == p.height && profile.width == p.width
                                                && profile.stream == p.stream;
                                        }));
                                    }

                                    // grab first lock
                                    CAPTURE(rs_subdevice(sub1));
                                    CAPTURE(modes1[0].stream);
                                    rs::streaming_lock lock1, lock2;
                                    REQUIRE_NOTHROW(lock1 = sub1.open(modes1[0]));

                                    SECTION("same mode") {
                                        // selected, but not streaming
                                        REQUIRE_THROWS_AS(lock2 = sub2.open({ modes1[0] }), rs::error);

                                        // streaming
                                        REQUIRE_NOTHROW(lock1.start([](rs_frame * fref) {}));
                                        REQUIRE_THROWS_AS(lock2 = sub2.open({ modes1[0] }), rs::error);
                                    }
                                    SECTION("different modes") {
                                        if (modes1.size() == 1) {
                                            WARN("device " << dev1.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME) << " S/N: " << dev1.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER) << " w/ FW v" << dev1.get_camera_info(RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION) << ":");
                                            WARN("subdevice " << rs_subdevice(sub1) << " has only 1 supported streaming mode. Skipping Same Subdevice, different modes test.");
                                        }
                                        else
                                        {
                                            // selected, but not streaming
                                            REQUIRE_THROWS_AS(lock2 = sub2.open({ modes1[1] }), rs::error);

                                            // streaming
                                            REQUIRE_NOTHROW(lock1.start([](rs_frame * fref) {}));
                                            REQUIRE_THROWS_AS(lock2 = sub2.open({ modes1[1] }), rs::error);
                                        }
                                    }
                                    REQUIRE_NOTHROW(lock1.stop());
                                }
                            }
                            else
                            {
                                SECTION("different subdevice") {
                                    rs::streaming_lock lock1;
                                    CAPTURE(rs_subdevice(sub1));
                                    CAPTURE(rs_subdevice(sub2));

                                    // get first lock
                                    REQUIRE_NOTHROW(lock1 = sub1.open(sub1.get_stream_modes()[0]));

                                    // selected, but not streaming
                                    {
                                        rs::streaming_lock lock2;
                                        CAPTURE(sub2.get_stream_modes()[0].stream);
                                        REQUIRE_NOTHROW(lock2 = sub2.open(sub2.get_stream_modes()[0]));
                                        REQUIRE_NOTHROW(lock2.start([](rs_frame * fref) {}));
                                        REQUIRE_NOTHROW(lock2.stop());
                                    }

                                    // streaming
                                    {
                                        rs::streaming_lock lock2;
                                        REQUIRE_NOTHROW(lock1.start([](rs_frame * fref) {}));
                                        REQUIRE_NOTHROW(lock2 = sub2.open(sub2.get_stream_modes()[0]));
                                        REQUIRE_NOTHROW(lock2.start([](rs_frame * fref) {}));
                                        // stop streaming in opposite order just to be sure that works too
                                        REQUIRE_NOTHROW(lock1.stop());
                                        REQUIRE_NOTHROW(lock2.stop());
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        SECTION("subdevices on separate devices") {
            if (list.size() == 1) {
                WARN("Only one device connected. Skipping multi-device test");
            }
            else
            {
                for (auto & dev1 : list) {
                    for (auto & dev2 : list2) {
                        if (dev1.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER)
                            == dev2.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER)
                            )
                            continue;
                        for (auto && sub1 : dev1) {
                            for (auto && sub2 : dev2) {
                                // get modes
                                std::vector<rs::stream_profile> modes1, modes2;
                                REQUIRE_NOTHROW(modes1 = sub1.get_stream_modes());
                                REQUIRE_NOTHROW(modes2 = sub2.get_stream_modes());
                                REQUIRE(modes1.size() > 0);
                                REQUIRE(modes2.size() > 0);

                                // grab first lock
                                CAPTURE(rs_subdevice(sub1));
                                CAPTURE(modes1[0].stream);
                                rs::streaming_lock lock1;
                                REQUIRE_NOTHROW(lock1 = sub1.open(modes1[0]));

                                // try to acquire second lock

                                // selected, but not streaming
                                {
                                    rs::streaming_lock lock2;
                                    REQUIRE_NOTHROW(lock2 = sub2.open({ modes1[1] }));
                                    REQUIRE_NOTHROW(lock2.start([](rs_frame * fref) {}));
                                    REQUIRE_NOTHROW(lock2.stop());
                                }

                                // streaming
                                {
                                    rs::streaming_lock lock2;
                                    REQUIRE_NOTHROW(lock1.start([](rs_frame * fref) {}));
                                    REQUIRE_NOTHROW(lock2 = sub2.open({ modes1[1] }));
                                    REQUIRE_NOTHROW(lock2.start([](rs_frame * fref) {}));
                                    // stop streaming in opposite order just to be sure that works too
                                    REQUIRE_NOTHROW(lock1.stop());
                                    REQUIRE_NOTHROW(lock2.stop());
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}