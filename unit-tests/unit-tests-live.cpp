// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This set of tests is valid for any number and combination of RealSense cameras, including R200 and F200 //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "unit-tests-common.h"

TEST_CASE("Device metadata enumerates correctly", "[live]")
{
    auto ctx = make_context(__FUNCTION__);
    // Require at least one device to be plugged in
    std::vector<rs::device> list;
    REQUIRE_NOTHROW(list = ctx.query_devices());
    const int device_count = list.size();
    REQUIRE(device_count > 0);

    // For each device
    for (auto&& dev : list)
    {
        SECTION("supported device metadata strings are nonempty, unsupported ones throw")
        {
            for (auto j = 0; j < RS_CAMERA_INFO_COUNT; ++j) {
                auto is_supported = false;
                REQUIRE_NOTHROW(is_supported = dev.supports(rs_camera_info(j)));
                if (is_supported) REQUIRE(dev.get_camera_info(rs_camera_info(j)) != nullptr);
                else REQUIRE_THROWS_AS(dev.get_camera_info(rs_camera_info(j)), rs::error);
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
    for (auto&& dev : list)
    {
        // Configure all supported streams to run at 30 frames per second
        auto streams = config.open(dev);

        for (auto i = 0; i < 5; i++)
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

TEST_CASE("no extrinsic transformation between a stream and itself", "[live]")
{
    // Require at least one device to be plugged in
    auto ctx = make_context(__FUNCTION__);
    std::vector<rs::device> list;
    REQUIRE_NOTHROW(list = ctx.query_devices());
    const int device_count = list.size();
    REQUIRE(device_count > 0);

    // For each device
    for (auto&& dev : list)
    {
        rs_extrinsics extrin;
        try {
            extrin = ctx.get_extrinsics(dev, dev);
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

TEST_CASE("extrinsic transformation between two streams is a rigid transform", "[live]")
{
    // Require at least one device to be plugged in
    auto ctx = make_context(__FUNCTION__);
    std::vector<rs::device> list;
    REQUIRE_NOTHROW(list = ctx.query_devices());
    const int device_count = list.size();
    REQUIRE(device_count > 0);

    // For each device
    for (int i = 0; i < device_count; ++i)
    {
        auto dev = list[i];
        auto adj_devices = dev.get_adjacent_devices();
        //REQUIRE(dev != nullptr);

        // For every pair of streams
        for (auto j = 0; j < adj_devices.size(); ++j)
        {
            for (int k = j + 1; k < adj_devices.size(); ++k)
            {
                // Extrinsics from A to B should have an orthonormal 3x3 rotation matrix and a translation vector of magnitude less than 10cm
                rs_extrinsics a_to_b;

                try {
                    a_to_b = ctx.get_extrinsics(adj_devices[j], adj_devices[k]);
                }
                catch (rs::error &e) {
                    WARN(e.what());
                    continue;
                }

                require_rotation_matrix(a_to_b.rotation);
                REQUIRE(vector_length(a_to_b.translation) < 0.1f);

                // Extrinsics from B to A should be the inverse of extrinsics from A to B
                rs_extrinsics b_to_a;
                REQUIRE_NOTHROW(b_to_a = ctx.get_extrinsics(adj_devices[k], adj_devices[j]));

                require_transposed(a_to_b.rotation, b_to_a.rotation);
                REQUIRE(b_to_a.rotation[0] * a_to_b.translation[0] + b_to_a.rotation[3] * a_to_b.translation[1] + b_to_a.rotation[6] * a_to_b.translation[2] == Approx(-b_to_a.translation[0]));
                REQUIRE(b_to_a.rotation[1] * a_to_b.translation[0] + b_to_a.rotation[4] * a_to_b.translation[1] + b_to_a.rotation[7] * a_to_b.translation[2] == Approx(-b_to_a.translation[1]));
                REQUIRE(b_to_a.rotation[2] * a_to_b.translation[0] + b_to_a.rotation[5] * a_to_b.translation[1] + b_to_a.rotation[8] * a_to_b.translation[2] == Approx(-b_to_a.translation[2]));
            }
        }
    }
}

TEST_CASE("extrinsic transformations are transitive", "[live]")
{
    // Require at least one device to be plugged in
    auto ctx = make_context(__FUNCTION__);
    std::vector<rs::device> list;
    REQUIRE_NOTHROW(list = ctx.query_devices());
    const int device_count = list.size();
    REQUIRE(device_count > 0);

    // For each device
    for (auto&& dev : list)
    {
        auto adj_devices = dev.get_adjacent_devices();

        // For every set of subdevices
        for (auto a = 0; a < adj_devices.size(); ++a)
        {
            for (auto b = 0; b < adj_devices.size(); ++b)
            {
                for (auto c = 0; c < adj_devices.size(); ++c)
                {
                    // Require that the composition of a_to_b and b_to_c is equal to a_to_c
                    rs_extrinsics a_to_b, b_to_c, a_to_c;

                    try {
                        a_to_b = ctx.get_extrinsics(adj_devices[a], adj_devices[b]);
                        b_to_c = ctx.get_extrinsics(adj_devices[b], adj_devices[c]);
                        a_to_c = ctx.get_extrinsics(adj_devices[a], adj_devices[c]);
                    }
                    catch (rs::error &e)
                    {
                        WARN(e.what());
                        continue;
                    }

                    // a_to_c.rotation == a_to_b.rotation * b_to_c.rotation
                    REQUIRE(a_to_c.rotation[0] == Approx(a_to_b.rotation[0] * b_to_c.rotation[0] + a_to_b.rotation[3] * b_to_c.rotation[1] + a_to_b.rotation[6] * b_to_c.rotation[2]));
                    REQUIRE(a_to_c.rotation[2] == Approx(a_to_b.rotation[2] * b_to_c.rotation[0] + a_to_b.rotation[5] * b_to_c.rotation[1] + a_to_b.rotation[8] * b_to_c.rotation[2]));
                    REQUIRE(a_to_c.rotation[1] == Approx(a_to_b.rotation[1] * b_to_c.rotation[0] + a_to_b.rotation[4] * b_to_c.rotation[1] + a_to_b.rotation[7] * b_to_c.rotation[2]));
                    REQUIRE(a_to_c.rotation[3] == Approx(a_to_b.rotation[0] * b_to_c.rotation[3] + a_to_b.rotation[3] * b_to_c.rotation[4] + a_to_b.rotation[6] * b_to_c.rotation[5]));
                    REQUIRE(a_to_c.rotation[4] == Approx(a_to_b.rotation[1] * b_to_c.rotation[3] + a_to_b.rotation[4] * b_to_c.rotation[4] + a_to_b.rotation[7] * b_to_c.rotation[5]));
                    REQUIRE(a_to_c.rotation[5] == Approx(a_to_b.rotation[2] * b_to_c.rotation[3] + a_to_b.rotation[5] * b_to_c.rotation[4] + a_to_b.rotation[8] * b_to_c.rotation[5]));
                    REQUIRE(a_to_c.rotation[6] == Approx(a_to_b.rotation[0] * b_to_c.rotation[6] + a_to_b.rotation[3] * b_to_c.rotation[7] + a_to_b.rotation[6] * b_to_c.rotation[8]));
                    REQUIRE(a_to_c.rotation[7] == Approx(a_to_b.rotation[1] * b_to_c.rotation[6] + a_to_b.rotation[4] * b_to_c.rotation[7] + a_to_b.rotation[7] * b_to_c.rotation[8]));
                    REQUIRE(a_to_c.rotation[8] == Approx(a_to_b.rotation[2] * b_to_c.rotation[6] + a_to_b.rotation[5] * b_to_c.rotation[7] + a_to_b.rotation[8] * b_to_c.rotation[8]));

                    // a_to_c.translation = a_to_b.transform(b_to_c.translation)
                    REQUIRE(a_to_c.translation[0] == Approx(a_to_b.rotation[0] * b_to_c.translation[0] + a_to_b.rotation[3] * b_to_c.translation[1] + a_to_b.rotation[6] * b_to_c.translation[2] + a_to_b.translation[0]));
                    REQUIRE(a_to_c.translation[1] == Approx(a_to_b.rotation[1] * b_to_c.translation[0] + a_to_b.rotation[4] * b_to_c.translation[1] + a_to_b.rotation[7] * b_to_c.translation[2] + a_to_b.translation[1]));
                    REQUIRE(a_to_c.translation[2] == Approx(a_to_b.rotation[2] * b_to_c.translation[0] + a_to_b.rotation[5] * b_to_c.translation[1] + a_to_b.rotation[8] * b_to_c.translation[2] + a_to_b.translation[2]));
                }
            }
        }
    }
}

// break up
TEST_CASE("streaming modes sanity check", "[live]")
{
    // Require at least one device to be plugged in
    auto ctx = make_context(__FUNCTION__);
    std::vector<rs::device> list;
    REQUIRE_NOTHROW(list = ctx.query_devices());
    const int device_count = list.size();
    REQUIRE(device_count > 0);

    // For each device
    for (auto&& dev : list)
    {
        // make sure they provide at least one streaming mode
        std::vector<rs::stream_profile> stream_profiles;
        REQUIRE_NOTHROW(stream_profiles = dev.get_stream_modes());
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
                REQUIRE_NOTHROW(dev.open({ profile }));
                // TODO: make callback confirm stream format/dimensions/framerate
                REQUIRE_NOTHROW(dev.start([](rs_frame * fref) {}));

                // Require that we can disable the stream afterwards
                REQUIRE_NOTHROW(dev.stop());
            }
            SECTION("check stream intrinsics are sane") {
                rs_intrinsics intrin;
                REQUIRE_NOTHROW(intrin = dev.get_intrinsics(profile));

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

TEST_CASE("check option API", "[live][options]")
{
    // Require at least one device to be plugged in
    auto ctx = make_context(__FUNCTION__);
    std::vector<rs::device> list;
    REQUIRE_NOTHROW(list = ctx.query_devices());
    REQUIRE(list.size() > 0);

    // for each device
    for (auto&& dev : list)
    {
        // for each option
        for (auto i = 0; i < RS_OPTION_COUNT; ++i) {
            auto opt = rs_option(i);
            bool is_opt_supported;
            REQUIRE_NOTHROW(is_opt_supported = dev.supports(opt));


            SECTION("Ranges are sane")
            {
                if (!is_opt_supported)
                {
                    REQUIRE_THROWS_AS(dev.get_option_range(opt), rs::error);
                }
                else
                {
                    rs::option_range range;
                    REQUIRE_NOTHROW(range = dev.get_option_range(opt));

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
            SECTION("get_option returns a legal value")
            {
                if (!is_opt_supported)
                {
                    REQUIRE_THROWS_AS(dev.get_option(opt), rs::error);
                }
                else
                {
                    auto range = dev.get_option_range(opt);
                    float value;
                    REQUIRE_NOTHROW(value = dev.get_option(opt));

                    // value in range. Do I need to account for epsilon in lt[e]/gt[e] comparisons?
                    REQUIRE(value >= range.min);
                    REQUIRE(value <= range.max);

                    // value doesn't change between two gets (if no additional threads are calling set)
                    REQUIRE(dev.get_option(opt) == Approx(value));

                    // REQUIRE(value == Approx(range.def)); // Not sure if this is a reasonable check
                    // TODO: make sure value == range.min + k*range.step for some k?
                }
            }
            SECTION("set opt doesn't like bad values") {
                if (!is_opt_supported)
                {
                    REQUIRE_THROWS_AS(dev.set_option(opt, 1), rs::error);
                }
                else
                {
                    auto range = dev.get_option_range(opt);

                    // minimum should work, as should maximum
                    REQUIRE_NOTHROW(dev.set_option(opt, range.min));
                    REQUIRE_NOTHROW(dev.set_option(opt, range.max));

                    int n_steps = (range.max - range.min) / range.step;

                    // check a few arbitrary points along the scale
                    REQUIRE_NOTHROW(dev.set_option(opt, range.min + (1 % n_steps)*range.step));
                    REQUIRE_NOTHROW(dev.set_option(opt, range.min + (11 % n_steps)*range.step));
                    REQUIRE_NOTHROW(dev.set_option(opt, range.min + (111 % n_steps)*range.step));
                    REQUIRE_NOTHROW(dev.set_option(opt, range.min + (1111 % n_steps)*range.step));

                    // below min and above max shouldn't work
                    REQUIRE_THROWS_AS(dev.set_option(opt, range.min - range.step), rs::error);
                    REQUIRE_THROWS_AS(dev.set_option(opt, range.max + range.step), rs::error);

                    // make sure requesting value in the range, but not a legal step doesn't work
                    // TODO: maybe something for range.step < 1 ?
                    for (auto j = 1; j < range.step; j++) {
                        CAPTURE(range.step);
                        CAPTURE(j);
                        REQUIRE_THROWS_AS(dev.set_option(opt, range.min + j), rs::error);
                    }
                }
            }
            SECTION("check get/set sequencing works as expected") {
                if (!is_opt_supported) continue;
                auto range = dev.get_option_range(opt);

                // setting a valid value lets you get that value back
                dev.set_option(opt, range.min);
                REQUIRE(dev.get_option(opt) == Approx(range.min));

                // setting an invalid value returns the last set valid value.
                REQUIRE_THROWS(dev.set_option(opt, range.max + range.step));
                REQUIRE(dev.get_option(opt) == Approx(range.min));

                dev.set_option(opt, range.max);
                REQUIRE_THROWS(dev.set_option(opt, range.min - range.step));
                REQUIRE(dev.get_option(opt) == Approx(range.max));

            }
            SECTION("get_description returns a non-empty, non-null string") {
                if (!is_opt_supported) {
                    REQUIRE_THROWS_AS(dev.get_option_description(opt), rs::error);
                }
                else
                {
                    REQUIRE(dev.get_option_description(opt) != nullptr);
                    REQUIRE(std::string(dev.get_option_description(opt)) != std::string(""));
                }
            }
            // TODO: tests for get_option_value_description? possibly too open a function to have useful tests
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

    SECTION("Single context")
    {
        SECTION("subdevices on a single device")
        {
            for (auto & dev : list)
            {
                SECTION("opening the same subdevice multiple times")
                {
                    auto modes = dev.get_stream_modes();
                    REQUIRE(modes.size() > 0);
                    CAPTURE(modes.front().stream);
                    REQUIRE_NOTHROW(dev.open(modes.front()));

                    SECTION("same mode")
                    {
                        // selected, but not streaming
                        REQUIRE_THROWS_AS(dev.open({ modes.front() }), rs::error);

                        // streaming
                        REQUIRE_NOTHROW(dev.start([](rs_frame * fref) {}));
                        REQUIRE_THROWS_AS(dev.open({ modes.front() }), rs::error);
                    }

                    SECTION("different modes")
                    {
                        if (modes.size() == 1)
                        {
                            WARN("device " << dev.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME) << " S/N: " << dev.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER) << " w/ FW v" << dev.get_camera_info(RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION) << ":");
                            WARN("subdevice has only 1 supported streaming mode. Skipping Same Subdevice, different modes test.");
                        }
                        else
                        {
                            // selected, but not streaming
                            REQUIRE_THROWS_AS(dev.open({ modes[1] }), rs::error);

                            // streaming
                            REQUIRE_NOTHROW(dev.start([](rs_frame * fref) {}));
                            REQUIRE_THROWS_AS(dev.open({ modes[1] }), rs::error);
                        }
                    }

                    REQUIRE_NOTHROW(dev.stop());
                }
                // TODO: Move
                SECTION("opening different subdevices") {
                    for (auto&& subdevice1 : dev.get_adjacent_devices()) 
                    {
                        for (auto&& subdevice2 : dev.get_adjacent_devices()) 
                        {
                            if (subdevice1 == subdevice2)
                                continue;

                            // get first lock
                            REQUIRE_NOTHROW(subdevice1.open(subdevice1.get_stream_modes().front()));

                            // selected, but not streaming
                            {
                                CAPTURE(subdevice2.get_stream_modes().front().stream);
                                REQUIRE_NOTHROW(subdevice2.open(subdevice2.get_stream_modes().front()));
                                REQUIRE_NOTHROW(subdevice2.start([](rs_frame * fref) {}));
                                REQUIRE_NOTHROW(subdevice2.stop());
                                REQUIRE_NOTHROW(subdevice2.close());
                            }

                            // streaming
                            {
                                REQUIRE_NOTHROW(subdevice1.start([](rs_frame * fref) {}));
                                REQUIRE_NOTHROW(subdevice2.open(subdevice2.get_stream_modes().front()));
                                REQUIRE_NOTHROW(subdevice2.start([](rs_frame * fref) {}));
                                // stop streaming in opposite order just to be sure that works too
                                REQUIRE_NOTHROW(subdevice1.stop());
                                REQUIRE_NOTHROW(subdevice2.stop());
                                REQUIRE_NOTHROW(subdevice2.close());
                            }

                            REQUIRE_NOTHROW(subdevice1.close());
                        }
                    }
                }
            }
        }
        SECTION("multiple devices")
        {
            if (list.size() == 1)
            {
                WARN("Only one device connected. Skipping multi-device test");
            }
            else
            {
                for (auto & dev1 : list)
                {
                    for (auto & dev2 : list)
                    {
                        // couldn't think of a better way to compare the two...
                        if (dev1 == dev2)
                            continue;

                        REQUIRE_NOTHROW(dev1.open(dev1.get_stream_modes().front()));
                        REQUIRE_NOTHROW(dev2.open(dev2.get_stream_modes().front()));

                        REQUIRE_NOTHROW(dev1.start([](rs_frame * fref) {}));
                        REQUIRE_NOTHROW(dev2.start([](rs_frame * fref) {}));
                        REQUIRE_NOTHROW(dev1.stop());
                        REQUIRE_NOTHROW(dev2.stop());

                        REQUIRE_NOTHROW(dev1.close());
                        REQUIRE_NOTHROW(dev2.close());
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
        SECTION("subdevices on a single device")
        {
            for (auto&& dev1 : list)
            {
                for (auto&& dev2 : list2)
                {
                    if (dev1 == dev2)
                    {
                        SECTION("same subdevice") {
                            // get modes
                            std::vector<rs::stream_profile> modes1, modes2;
                            REQUIRE_NOTHROW(modes1 = dev1.get_stream_modes());
                            REQUIRE_NOTHROW(modes2 = dev2.get_stream_modes());
                            REQUIRE(modes1.size() > 0);
                            REQUIRE(modes1.size() == modes2.size());
                            // require that the lists are the same (disregarding order)
                            for (auto profile : modes1) {
                                REQUIRE(std::any_of(begin(modes2), end(modes2), [&](const rs::stream_profile & p)
                                {
                                    return profile == p;
                                }));
                            }

                            // grab first lock
                            CAPTURE(modes1.front().stream);
                            REQUIRE_NOTHROW(dev1.open(modes1.front()));

                            SECTION("same mode")
                            {
                                // selected, but not streaming
                                REQUIRE_THROWS_AS(dev2.open({ modes1.front() }), rs::error);

                                // streaming
                                REQUIRE_NOTHROW(dev1.start([](rs_frame * fref) {}));
                                REQUIRE_THROWS_AS(dev2.open({ modes1.front() }), rs::error);
                            }
                            SECTION("different modes")
                            {
                                if (modes1.size() == 1)
                                {
                                    WARN("device " << dev1.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME) << " S/N: " << dev1.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER) << " w/ FW v" << dev1.get_camera_info(RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION) << ":");
                                    WARN("Device has only 1 supported streaming mode. Skipping Same Subdevice, different modes test.");
                                }
                                else
                                {
                                    // selected, but not streaming
                                    REQUIRE_THROWS_AS(dev2.open({ modes1[1] }), rs::error);

                                    // streaming
                                    REQUIRE_NOTHROW(dev1.start([](rs_frame * fref) {}));
                                    REQUIRE_THROWS_AS(dev2.open({ modes1[1] }), rs::error);
                                }
                            }
                            REQUIRE_NOTHROW(dev1.stop());
                        }
                    }
                    else
                    {
                        SECTION("different subdevice")
                        {
                            // get first lock
                            REQUIRE_NOTHROW(dev1.open(dev1.get_stream_modes().front()));

                            // selected, but not streaming
                            {
                                CAPTURE(dev2.get_stream_modes().front().stream);
                                REQUIRE_NOTHROW(dev2.open(dev2.get_stream_modes().front()));
                                REQUIRE_NOTHROW(dev2.start([](rs_frame * fref) {}));
                                REQUIRE_NOTHROW(dev2.stop());
                                REQUIRE_NOTHROW(dev2.close());
                            }

                            // streaming
                            {
                                REQUIRE_NOTHROW(dev1.start([](rs_frame * fref) {}));
                                REQUIRE_NOTHROW(dev2.open(dev2.get_stream_modes().front()));
                                REQUIRE_NOTHROW(dev2.start([](rs_frame * fref) {}));
                                // stop streaming in opposite order just to be sure that works too
                                REQUIRE_NOTHROW(dev1.stop());
                                REQUIRE_NOTHROW(dev2.stop());

                                REQUIRE_NOTHROW(dev1.close());
                                REQUIRE_NOTHROW(dev2.close());
                            }
                        }
                    }
                }
            }
        }
        SECTION("subdevices on separate devices")
        {
            if (list.size() == 1)
            {
                WARN("Only one device connected. Skipping multi-device test");
            }
            else
            {
                for (auto & dev1 : list)
                {
                    for (auto & dev2 : list2)
                    {

                        if (dev1 == dev2)
                            continue;

                        // get modes
                        std::vector<rs::stream_profile> modes1, modes2;
                        REQUIRE_NOTHROW(modes1 = dev1.get_stream_modes());
                        REQUIRE_NOTHROW(modes2 = dev2.get_stream_modes());
                        REQUIRE(modes1.size() > 0);
                        REQUIRE(modes2.size() > 0);

                        // grab first lock
                        CAPTURE(modes1.front().stream);
                        CAPTURE(dev1.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME));
                        CAPTURE(dev1.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER));
                        CAPTURE(dev2.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME));
                        CAPTURE(dev2.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER));
                        REQUIRE_NOTHROW(dev1.open(modes1.front()));

                        // try to acquire second lock

                        // selected, but not streaming
                        {
                            REQUIRE_NOTHROW(dev2.open({ modes2.front() }));
                            REQUIRE_NOTHROW(dev2.start([](rs_frame * fref) {}));
                            REQUIRE_NOTHROW(dev2.stop());
                            REQUIRE_NOTHROW(dev2.close());
                        }

                        // streaming
                        {
                            REQUIRE_NOTHROW(dev1.start([](rs_frame * fref) {}));
                            REQUIRE_NOTHROW(dev2.open({ modes2.front() }));
                            REQUIRE_NOTHROW(dev2.start([](rs_frame * fref) {}));
                            // stop streaming in opposite order just to be sure that works too
                            REQUIRE_NOTHROW(dev1.stop());
                            REQUIRE_NOTHROW(dev2.stop());
                            REQUIRE_NOTHROW(dev2.close());
                        }

                        REQUIRE_NOTHROW(dev1.close());
                    }
                }
            }
        }
    }
}

TEST_CASE("All suggested profiles can be opened", "[live]") {
    // Require at least one device to be plugged in
    auto ctx = make_context(__FUNCTION__);
    std::vector<rs::device> list;
    REQUIRE_NOTHROW(list = ctx.query_devices());
    REQUIRE(list.size() > 0);

    for (auto && subdevice : list) {
        std::vector<rs::stream_profile> modes;
        REQUIRE_NOTHROW(modes = subdevice.get_stream_modes());

        REQUIRE(modes.size() > 0);
        WARN(subdevice.get_camera_info(RS_CAMERA_INFO_MODULE_NAME));
        for (int i = 0; i < modes.size(); i+=1) {
            //CAPTURE(rs_subdevice(subdevice));
            CAPTURE(modes[i].format);
            CAPTURE(modes[i].fps);
            CAPTURE(modes[i].height);
            CAPTURE(modes[i].width);
            CAPTURE(modes[i].stream);

            REQUIRE_NOTHROW(subdevice.open({ modes[i] }));
            REQUIRE_NOTHROW(subdevice.start([](rs_frame * fref) {}));
            REQUIRE_NOTHROW(subdevice.stop());
            REQUIRE_NOTHROW(subdevice.close());
        }
    }
}

class AC_Mock_Device
{
public:
    AC_Mock_Device(std::vector<rs::stream_profile> modes, bool result) : result(result), modes(std::move(modes)), expected() {};
    void set_expected(std::vector<rs::stream_profile> profiles) { expected = profiles; };

    std::vector<AC_Mock_Device> get_adjacent_devices() const { return{ *this }; }
    std::vector<rs::stream_profile> get_stream_modes() const { return modes; };
    bool open(std::vector<rs::stream_profile> profiles) const {
        for (auto & profile : profiles) {
            if (profile.has_wildcards()) return false;
            if (std::none_of(begin(expected), end(expected), [&profile](const rs::stream_profile &p) {return p.match(profile); }))
                return false;
        }
        return true;
    }

    rs_intrinsics get_intrinsics(rs::stream_profile profile) const {
        return{ 0, 0, 0.0, 0.0, 0.0, 0.0, RS_DISTORTION_COUNT,{ 0.0, 0.0, 0.0, 0.0, 0.0 } };
    }
    const char * get_camera_info(rs_camera_info info) const {
        switch (info) {
        case RS_CAMERA_INFO_MODULE_NAME: return "Dummy Module";
        default: return "";
        }
    }

    template<class T>
    void start(T callback) const
    {
        REQUIRE(result == true);
    }

private:
    std::vector<rs::stream_profile> modes, expected;
    bool result;
};

TEST_CASE("Auto-complete feature works", "[offline][rs::util::config]") {
    // dummy device can provide the following profiles:
    AC_Mock_Device dev({ { RS_STREAM_DEPTH   , 640, 240,  10, RS_FORMAT_Z16 },
                      { RS_STREAM_DEPTH   , 640, 240,  30, RS_FORMAT_Z16 },
                      { RS_STREAM_DEPTH   , 640, 240, 110, RS_FORMAT_Z16 },
                      { RS_STREAM_DEPTH   , 640, 480,  10, RS_FORMAT_Z16 },
                      { RS_STREAM_DEPTH   , 640, 480,  30, RS_FORMAT_Z16 },
                      { RS_STREAM_INFRARED, 640, 240,  10, RS_FORMAT_Y8  },
                      { RS_STREAM_INFRARED, 640, 240,  30, RS_FORMAT_Y8  },
                      { RS_STREAM_INFRARED, 640, 240, 110, RS_FORMAT_Y8  },
                      { RS_STREAM_INFRARED, 640, 480,  10, RS_FORMAT_Y8  },
                      { RS_STREAM_INFRARED, 640, 480,  30, RS_FORMAT_Y8  },
                      { RS_STREAM_INFRARED, 640, 480, 200, RS_FORMAT_Y8  } }, true);
    rs::util::Config<AC_Mock_Device> config;
    
    struct Test { 
        std::vector<rs::stream_profile> given,       // We give these profiles to the config class
                                        expected;    // pool of profiles the config class can return. Leave empty if auto-completer is expected to fail
    };
    std::vector<Test> tests = {
        // Test 0 (Depth always has RS_FORMAT_Z16)
        { { { RS_STREAM_DEPTH   ,   0,   0,   0, RS_FORMAT_ANY } },   // given
          { { RS_STREAM_DEPTH   ,   0,   0,   0, RS_FORMAT_Z16 } } }, // expected
        // Test 1 (IR always has RS_FORMAT_Y8)
        { { { RS_STREAM_INFRARED,   0,   0,   0, RS_FORMAT_ANY } },   // given
          { { RS_STREAM_INFRARED,   0,   0,   0, RS_FORMAT_Y8  } } }, // expected
        // Test 2 (No 200 fps depth)
        { { { RS_STREAM_DEPTH   ,   0,   0, 200, RS_FORMAT_ANY } },   // given
          { } },                                                      // expected
        // Test 3 (Can request 200 fps IR)
        { { { RS_STREAM_INFRARED,   0,   0, 200, RS_FORMAT_ANY } },   // given
          { { RS_STREAM_INFRARED,   0,   0, 200, RS_FORMAT_ANY } } }, // expected
        // Test 4 (requesting IR@200fps + depth fails
        { { { RS_STREAM_INFRARED,   0,   0, 200, RS_FORMAT_ANY }, { RS_STREAM_DEPTH   ,   0,   0,   0, RS_FORMAT_ANY } },   // given
          { } },                                                                                                            // expected
        // Test 5 (Can't do 640x480@110fps a)
        { { { RS_STREAM_INFRARED, 640, 480, 110, RS_FORMAT_ANY } },   // given
          { } },                                                      // expected
        // Test 6 (Can't do 640x480@110fps b)
        { { { RS_STREAM_DEPTH   , 640, 480,   0, RS_FORMAT_ANY }, { RS_STREAM_INFRARED,   0,   0, 110, RS_FORMAT_ANY } },   // given
          { } },                                                                                                            // expected
        // Test 7 (Pull extra details from second stream a)
        { { { RS_STREAM_DEPTH   , 640, 480,   0, RS_FORMAT_ANY }, { RS_STREAM_INFRARED,   0,   0,  30, RS_FORMAT_ANY } },   // given
          { { RS_STREAM_DEPTH   , 640, 480,  30, RS_FORMAT_ANY }, { RS_STREAM_INFRARED, 640, 480,  30, RS_FORMAT_ANY } } }, // expected
        // Test 8 (Pull extra details from second stream b) [IR also supports 200, could fail if that gets selected]
        { { { RS_STREAM_INFRARED, 640, 480,   0, RS_FORMAT_ANY }, { RS_STREAM_DEPTH   ,   0,   0,   0, RS_FORMAT_ANY } },   // given
          { { RS_STREAM_INFRARED, 640, 480,  10, RS_FORMAT_ANY }, { RS_STREAM_INFRARED, 640, 480,  30, RS_FORMAT_ANY },     // expected - options for IR stream
            { RS_STREAM_DEPTH   , 640, 480,  10, RS_FORMAT_ANY }, { RS_STREAM_DEPTH   , 640, 480,  30, RS_FORMAT_ANY } } }  // expected - options for depth stream
    };

    for (int i=0; i<tests.size(); ++i)
    {
        config.disable_all();
        for (auto & profile : tests[i].given) {
            config.enable_stream(profile.stream, profile.width, profile.height, profile.fps, profile.format);
        }
        dev.set_expected(tests[i].expected);
        CAPTURE(i);
        if (tests[i].expected.size() == 0) {
            REQUIRE_THROWS_AS(config.open(dev), std::runtime_error);
        }
        else
        {
            rs::util::Config<AC_Mock_Device>::multistream results;
            REQUIRE_NOTHROW(results = config.open(dev));
            // REQUIRE()s are in here
            results.start(0);
        }
    }
}
