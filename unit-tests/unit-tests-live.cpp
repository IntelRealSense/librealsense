// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This set of tests is valid for any number and combination of RealSense cameras, including R200 and F200 //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include "unit-tests-common.h"
#include <librealsense/rsutil2.hpp>
#include <librealsense/rs2_advanced_mode.hpp>

using namespace rs2;

# define SECTION_FROM_TEST_NAME space_to_underscore(Catch::getCurrentContext().getResultCapture()->getCurrentTestName()).c_str()
//// disable in one place options that are sensitive to frame content
//// this is done to make sure unit-tests are deterministic
void disable_sensitive_options_for(sensor& dev)
{
    if (dev.supports(RS2_OPTION_ERROR_POLLING_ENABLED))
        REQUIRE_NOTHROW(dev.set_option(RS2_OPTION_ERROR_POLLING_ENABLED, 0));

    if (dev.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
        REQUIRE_NOTHROW(dev.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 0));

    if (dev.supports(RS2_OPTION_EXPOSURE))
    {
        auto range = dev.get_option_range(RS2_OPTION_EXPOSURE);
        REQUIRE_NOTHROW(dev.set_option(RS2_OPTION_EXPOSURE, range.def));
    }
}

void disable_sensitive_options_for(device& dev)
{
    for (auto&& s : dev.query_sensors())
        disable_sensitive_options_for(s);
}

TEST_CASE("Device metadata enumerates correctly", "[live]")
{
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {       // Require at least one device to be plugged in
        std::vector<device> list;
        REQUIRE_NOTHROW(list = ctx.query_devices());
        REQUIRE(list.size() > 0);

        // For each device
        for (auto&& dev : list)
        {
            SECTION("supported device metadata strings are nonempty, unsupported ones throw")
            {
                for (auto j = 0; j < RS2_CAMERA_INFO_COUNT; ++j) {
                    auto is_supported = false;
                    REQUIRE_NOTHROW(is_supported = dev.supports(rs2_camera_info(j)));
                    if (is_supported) REQUIRE(dev.get_info(rs2_camera_info(j)) != nullptr);
                    else REQUIRE_THROWS_AS(dev.get_info(rs2_camera_info(j)), error);
                }
            }
        }

    }
}


////////////////////////////////////////////////////////////
////// Test basic streaming functionality //
////////////////////////////////////////////////////////////
TEST_CASE("Start-Stop stream sequence", "[live]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {

        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        util::config config;
        REQUIRE_NOTHROW(config.enable_all(preset::best_quality));

        // For each device
        for (auto&& dev : list)
        {
            disable_sensitive_options_for(dev);

            // Configure all supported streams to run at 30 frames per second
            rs2::util::config::multistream streams;
            REQUIRE_NOTHROW(streams = config.open(ctx.get_sensor_parent(dev)));

            for (auto i = 0; i < 5; i++)
            {
                // Test sequence
                REQUIRE_NOTHROW(streams.start([](frame fref) {}));
                REQUIRE_NOTHROW(streams.stop());
            }

            config.disable_all();
        }
    }
}

///////////////////////////////////////
////// Calibration information tests //
///////////////////////////////////////

TEST_CASE("no extrinsic transformation between a stream and itself", "[live]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        const size_t device_count = list.size();
        REQUIRE(device_count > 0);

        // For each device
        for (auto&& dev : list)
        {
            rs2_extrinsics extrin;
            try {
                extrin = ctx.get_extrinsics(dev, dev);
            }
            catch (error &e) {
                // if device isn't calibrated, get_extrinsics must error out (according to old comment. Might not be true under new API)
                WARN(e.what());
                continue;
            }

            require_identity_matrix(extrin.rotation);
            require_zero_vector(extrin.translation);
        }
    }
}

TEST_CASE("extrinsic transformation between two streams is a rigid transform", "[live]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        const size_t device_count = list.size();
        REQUIRE(device_count > 0);

        // For each device
        for (int i = 0; i < device_count; ++i)
        {
            auto dev = list[i];
            auto adj_devices = ctx.get_sensor_parent(dev).query_sensors();
            //REQUIRE(dev != nullptr);

            // For every pair of streams
            for (auto j = 0; j < adj_devices.size(); ++j)
            {
                for (int k = j + 1; k < adj_devices.size(); ++k)
                {
                    // Extrinsics from A to B should have an orthonormal 3x3 rotation matrix and a translation vector of magnitude less than 10cm
                    rs2_extrinsics a_to_b;

                    try {
                        a_to_b = ctx.get_extrinsics(adj_devices[j], adj_devices[k]);
                    }
                    catch (error &e) {
                        WARN(e.what());
                        continue;
                    }

                    require_rotation_matrix(a_to_b.rotation);
                    REQUIRE(vector_length(a_to_b.translation) < 0.1f);

                    // Extrinsics from B to A should be the inverse of extrinsics from A to B
                    rs2_extrinsics b_to_a;
                    REQUIRE_NOTHROW(b_to_a = ctx.get_extrinsics(adj_devices[k], adj_devices[j]));

                    require_transposed(a_to_b.rotation, b_to_a.rotation);
                    REQUIRE(b_to_a.rotation[0] * a_to_b.translation[0] + b_to_a.rotation[3] * a_to_b.translation[1] + b_to_a.rotation[6] * a_to_b.translation[2] == Approx(-b_to_a.translation[0]));
                    REQUIRE(b_to_a.rotation[1] * a_to_b.translation[0] + b_to_a.rotation[4] * a_to_b.translation[1] + b_to_a.rotation[7] * a_to_b.translation[2] == Approx(-b_to_a.translation[1]));
                    REQUIRE(b_to_a.rotation[2] * a_to_b.translation[0] + b_to_a.rotation[5] * a_to_b.translation[1] + b_to_a.rotation[8] * a_to_b.translation[2] == Approx(-b_to_a.translation[2]));
                }
            }
        }
    }
}

TEST_CASE("extrinsic transformations are transitive", "[live]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        // For each device
        for (auto&& dev : list)
        {
            auto adj_devices = ctx.get_sensor_parent(dev).query_sensors();

            // For every set of subdevices
            for (auto a = 0; a < adj_devices.size(); ++a)
            {
                for (auto b = 0; b < adj_devices.size(); ++b)
                {
                    for (auto c = 0; c < adj_devices.size(); ++c)
                    {
                        // Require that the composition of a_to_b and b_to_c is equal to a_to_c
                        rs2_extrinsics a_to_b, b_to_c, a_to_c;

                        try {
                            a_to_b = ctx.get_extrinsics(adj_devices[a], adj_devices[b]);
                            b_to_c = ctx.get_extrinsics(adj_devices[b], adj_devices[c]);
                            a_to_c = ctx.get_extrinsics(adj_devices[a], adj_devices[c]);
                        }
                        catch (error &e)
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
}

TEST_CASE("check width and height of stream intrinsics", "[live]")
{
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        for (auto&& dev : list)
        {
            auto module_name = dev.get_info(RS2_CAMERA_INFO_NAME);
            // TODO: if FE
            std::vector<stream_profile> stream_profiles;
            REQUIRE_NOTHROW(stream_profiles = dev.get_stream_modes());
            REQUIRE(stream_profiles.size() > 0);

            // for each stream profile provided:
            for (auto& profile : stream_profiles)
            {
                for (auto& profile : stream_profiles)
                {
                    if (profile.stream == RS2_STREAM_GPIO1 ||
                        profile.stream == RS2_STREAM_GPIO2 ||
                        profile.stream == RS2_STREAM_GPIO3 ||
                        profile.stream == RS2_STREAM_GPIO4 ||
                        profile.stream == RS2_STREAM_GYRO  ||
                        profile.stream == RS2_STREAM_ACCEL)
                        continue;

                rs2_intrinsics intrin;
                REQUIRE_NOTHROW(intrin = dev.get_intrinsics(profile));

                // Intrinsic width/height must match width/height of streaming mode we requested
                REQUIRE(intrin.width == profile.width);
                REQUIRE(intrin.height == profile.height);
                }
            }
        }
    }
}

// break up
TEST_CASE("streaming modes sanity check", "[live]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        // For each device
        for (auto&& dev : list)
        {
            disable_sensitive_options_for(dev);

            // make sure they provide at least one streaming mode
            std::vector<stream_profile> stream_profiles;
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
                    REQUIRE(profile.format > RS2_FORMAT_ANY);
                    REQUIRE(profile.format < RS2_FORMAT_COUNT);
                    REQUIRE(profile.fps >= 2);
                    REQUIRE(profile.fps <= 300);

                    // require that we can start streaming this mode
                    REQUIRE_NOTHROW(dev.open({ profile }));
                    // TODO: make callback confirm stream format/dimensions/framerate
                    REQUIRE_NOTHROW(dev.start([](frame fref) {}));

                    // Require that we can disable the stream afterwards
                    REQUIRE_NOTHROW(dev.stop());
                    REQUIRE_NOTHROW(dev.close());
                }
                SECTION("check stream intrinsics are sane") {
                    rs2_intrinsics intrin;
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
}

TEST_CASE("motion profiles sanity", "[live]")
{
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        // For each device
        for (auto&& dev : list)
        {
            disable_sensitive_options_for(dev);

            // make sure they provide at least one streaming mode
            std::vector<stream_profile> stream_profiles;
            REQUIRE_NOTHROW(stream_profiles = dev.get_stream_modes());
            REQUIRE(stream_profiles.size() > 0);

            // for each stream profile provided:
            for (auto profile : stream_profiles)
            {
                SECTION("check motion intrisics") {

                    auto stream = profile.stream;
                    rs2_motion_device_intrinsic mm_int;

                    CAPTURE(stream);

                    if (stream != RS2_STREAM_ACCEL && stream != RS2_STREAM_GYRO)
                    {
                        REQUIRE_THROWS(dev.get_motion_intrinsics(stream));
                    }
                    else
                    {
                        REQUIRE_NOTHROW(mm_int = dev.get_motion_intrinsics(stream));

                        for (int j = 0; j < 3; j++)
                        {
                            auto scale = mm_int.data[j][j];
                            CAPTURE(scale);
                            // Make sure scale value is "sane"
                            // We don't expect Motion Device to require adjustment of more then 20%
                            REQUIRE(scale > 0.8);
                            REQUIRE(scale < 1.2);

                            auto bias = mm_int.data[0][3];
                            CAPTURE(bias);
                            // Make sure bias is "sane"
                            REQUIRE(bias > -0.5);
                            REQUIRE(bias < 0.5);
                        }
                    }
                }
            }
        }
    }
}

TEST_CASE("check option API", "[live][options]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        // for each device
        for (auto&& dev : list)
        {

            // for each option
            for (auto i = 0; i < RS2_OPTION_COUNT; ++i) {
                auto opt = rs2_option(i);
                bool is_opt_supported;
                REQUIRE_NOTHROW(is_opt_supported = dev.supports(opt));


                SECTION("Ranges are sane")
                {
                    if (!is_opt_supported)
                    {
                        REQUIRE_THROWS_AS(dev.get_option_range(opt), error);
                    }
                    else
                    {
                        option_range range;
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
                        REQUIRE_THROWS_AS(dev.get_option(opt), error);
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
                        REQUIRE_THROWS_AS(dev.set_option(opt, 1), error);
                    }
                    else
                    {
                        auto range = dev.get_option_range(opt);

                        // minimum should work, as should maximum
                        REQUIRE_NOTHROW(dev.set_option(opt, range.min));
                        REQUIRE_NOTHROW(dev.set_option(opt, range.max));

                        int n_steps = int((range.max - range.min) / range.step);

                        // check a few arbitrary points along the scale
                        REQUIRE_NOTHROW(dev.set_option(opt, range.min + (1 % n_steps)*range.step));
                        REQUIRE_NOTHROW(dev.set_option(opt, range.min + (11 % n_steps)*range.step));
                        REQUIRE_NOTHROW(dev.set_option(opt, range.min + (111 % n_steps)*range.step));
                        REQUIRE_NOTHROW(dev.set_option(opt, range.min + (1111 % n_steps)*range.step));

                        // below min and above max shouldn't work
                        REQUIRE_THROWS_AS(dev.set_option(opt, range.min - range.step), error);
                        REQUIRE_THROWS_AS(dev.set_option(opt, range.max + range.step), error);

                        // make sure requesting value in the range, but not a legal step doesn't work
                        // TODO: maybe something for range.step < 1 ?
                        for (auto j = 1; j < range.step; j++) {
                            CAPTURE(range.step);
                            CAPTURE(j);
                            REQUIRE_THROWS_AS(dev.set_option(opt, range.min + j), error);
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
                        REQUIRE_THROWS_AS(dev.get_option_description(opt), error);
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
}

TEST_CASE("a single subdevice can only be opened once, different subdevices can be opened simultaneously", "[live][multicam]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        SECTION("Single context")
        {
            SECTION("subdevices on a single device")
            {
                for (auto & dev : list)
                {
                    disable_sensitive_options_for(dev);

                    SECTION("opening the same subdevice multiple times")
                    {
                        auto modes = dev.get_stream_modes();
                        REQUIRE(modes.size() > 0);
                        CAPTURE(modes.front().stream);
                        REQUIRE_NOTHROW(dev.open(modes.front()));

                        SECTION("same mode")
                        {
                            // selected, but not streaming
                            REQUIRE_THROWS_AS(dev.open({ modes.front() }), error);

                            // streaming
                            REQUIRE_NOTHROW(dev.start([](frame fref) {}));
                            REQUIRE_THROWS_AS(dev.open({ modes.front() }), error);
                        }

                        SECTION("different modes")
                        {
                            if (modes.size() == 1)
                            {
                                WARN("device " << dev.get_info(RS2_CAMERA_INFO_NAME) << " S/N: " << dev.get_info(
                                        RS2_CAMERA_INFO_SERIAL_NUMBER) << " w/ FW v" << dev.get_info(
                                        RS2_CAMERA_INFO_FIRMWARE_VERSION) << ":");
                                WARN("subdevice has only 1 supported streaming mode. Skipping Same Subdevice, different modes test.");
                            }
                            else
                            {
                                // selected, but not streaming
                                REQUIRE_THROWS_AS(dev.open({ modes[1] }), error);

                                // streaming
                                REQUIRE_NOTHROW(dev.start([](frame fref) {}));
                                REQUIRE_THROWS_AS(dev.open({ modes[1] }), error);
                            }
                        }

                        REQUIRE_NOTHROW(dev.stop());
                    }
                    // TODO: Move
                    SECTION("opening different subdevices") {
                        for (auto&& subdevice1 : ctx.get_sensor_parent(dev).query_sensors())
                        {
                            disable_sensitive_options_for(subdevice1);
                            for (auto&& subdevice2 : ctx.get_sensor_parent(dev).query_sensors())
                            {
                                disable_sensitive_options_for(subdevice2);

                                if (subdevice1 == subdevice2)
                                    continue;

                                // get first lock
                                REQUIRE_NOTHROW(subdevice1.open(subdevice1.get_stream_modes().front()));

                                // selected, but not streaming
                                {
                                    CAPTURE(subdevice2.get_stream_modes().front().stream);
                                    REQUIRE_NOTHROW(subdevice2.open(subdevice2.get_stream_modes().front()));
                                    REQUIRE_NOTHROW(subdevice2.start([](frame fref) {}));
                                    REQUIRE_NOTHROW(subdevice2.stop());
                                    REQUIRE_NOTHROW(subdevice2.close());
                                }

                                // streaming
                                {
                                    REQUIRE_NOTHROW(subdevice1.start([](frame fref) {}));
                                    REQUIRE_NOTHROW(subdevice2.open(subdevice2.get_stream_modes().front()));
                                    REQUIRE_NOTHROW(subdevice2.start([](frame fref) {}));
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
                        disable_sensitive_options_for(dev1);
                        for (auto & dev2 : list)
                        {
                            disable_sensitive_options_for(dev2);

                            // couldn't think of a better way to compare the two...
                            if (dev1 == dev2)
                                continue;

                            REQUIRE_NOTHROW(dev1.open(dev1.get_stream_modes().front()));
                            REQUIRE_NOTHROW(dev2.open(dev2.get_stream_modes().front()));

                            REQUIRE_NOTHROW(dev1.start([](frame fref) {}));
                            REQUIRE_NOTHROW(dev2.start([](frame fref) {}));
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
            rs2::context ctx2;
            REQUIRE(make_context("two_contexts", &ctx2));
            std::vector<sensor> list2;
            REQUIRE_NOTHROW(list2 = ctx2.query_all_sensors());
            REQUIRE(list2.size() == list.size());
            SECTION("subdevices on a single device")
            {
                for (auto&& dev1 : list)
                {
                    disable_sensitive_options_for(dev1);
                    for (auto&& dev2 : list2)
                    {
                        disable_sensitive_options_for(dev2);

                        if (dev1 == dev2)
                        {
                            SECTION("same subdevice") {
                                // get modes
                                std::vector<stream_profile> modes1, modes2;
                                REQUIRE_NOTHROW(modes1 = dev1.get_stream_modes());
                                REQUIRE_NOTHROW(modes2 = dev2.get_stream_modes());
                                REQUIRE(modes1.size() > 0);
                                REQUIRE(modes1.size() == modes2.size());
                                // require that the lists are the same (disregarding order)
                                for (auto profile : modes1) {
                                    REQUIRE(std::any_of(begin(modes2), end(modes2), [&profile](const stream_profile & p)
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
                                    REQUIRE_THROWS_AS(dev2.open({ modes1.front() }), error);

                                    // streaming
                                    REQUIRE_NOTHROW(dev1.start([](frame fref) {}));
                                    REQUIRE_THROWS_AS(dev2.open({ modes1.front() }), error);
                                }
                                SECTION("different modes")
                                {
                                    if (modes1.size() == 1)
                                    {
                                        WARN("device " << dev1.get_info(RS2_CAMERA_INFO_NAME) << " S/N: " << dev1.get_info(
                                                RS2_CAMERA_INFO_SERIAL_NUMBER) << " w/ FW v" << dev1.get_info(
                                                RS2_CAMERA_INFO_FIRMWARE_VERSION) << ":");
                                        WARN("Device has only 1 supported streaming mode. Skipping Same Subdevice, different modes test.");
                                    }
                                    else
                                    {
                                        // selected, but not streaming
                                        REQUIRE_THROWS_AS(dev2.open({ modes1[1] }), error);

                                        // streaming
                                        REQUIRE_NOTHROW(dev1.start([](frame fref) {}));
                                        REQUIRE_THROWS_AS(dev2.open({ modes1[1] }), error);
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
                                    REQUIRE_NOTHROW(dev2.start([](frame fref) {}));
                                    REQUIRE_NOTHROW(dev2.stop());
                                    REQUIRE_NOTHROW(dev2.close());
                                }

                                // streaming
                                {
                                    REQUIRE_NOTHROW(dev1.start([](frame fref) {}));
                                    REQUIRE_NOTHROW(dev2.open(dev2.get_stream_modes().front()));
                                    REQUIRE_NOTHROW(dev2.start([](frame fref) {}));
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
                        disable_sensitive_options_for(dev1);
                        for (auto & dev2 : list2)
                        {
                            disable_sensitive_options_for(dev2);

                            if (dev1 == dev2)
                                continue;

                            // get modes
                            std::vector<stream_profile> modes1, modes2;
                            REQUIRE_NOTHROW(modes1 = dev1.get_stream_modes());
                            REQUIRE_NOTHROW(modes2 = dev2.get_stream_modes());
                            REQUIRE(modes1.size() > 0);
                            REQUIRE(modes2.size() > 0);

                            // grab first lock
                            CAPTURE(modes1.front().stream);
                            CAPTURE(dev1.get_info(RS2_CAMERA_INFO_NAME));
                            CAPTURE(dev1.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
                            CAPTURE(dev2.get_info(RS2_CAMERA_INFO_NAME));
                            CAPTURE(dev2.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
                            REQUIRE_NOTHROW(dev1.open(modes1.front()));

                            // try to acquire second lock

                            // selected, but not streaming
                            {
                                REQUIRE_NOTHROW(dev2.open({ modes2.front() }));
                                REQUIRE_NOTHROW(dev2.start([](frame fref) {}));
                                REQUIRE_NOTHROW(dev2.stop());
                                REQUIRE_NOTHROW(dev2.close());
                            }

                            // streaming
                            {
                                REQUIRE_NOTHROW(dev1.start([](frame fref) {}));
                                REQUIRE_NOTHROW(dev2.open({ modes2.front() }));
                                REQUIRE_NOTHROW(dev2.start([](frame fref) {}));
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
}

TEST_CASE("All suggested profiles can be opened", "[live]") {

    // Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {

        const int num_of_profiles_for_each_subdevice = 2;

        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        for (auto && subdevice : list) {

            disable_sensitive_options_for(subdevice);

            std::vector<stream_profile> modes;
            REQUIRE_NOTHROW(modes = subdevice.get_stream_modes());

            REQUIRE(modes.size() > 0);
            WARN(subdevice.get_info(RS2_CAMERA_INFO_NAME));
            //the test will be done only on sub set of profile for each sub device
            for (int i = 0; i < modes.size(); i += (int)std::ceil((float)modes.size() / (float)num_of_profiles_for_each_subdevice)) {
                //CAPTURE(rs2_subdevice(subdevice));
                CAPTURE(modes[i].format);
                CAPTURE(modes[i].fps);
                CAPTURE(modes[i].height);
                CAPTURE(modes[i].width);
                CAPTURE(modes[i].stream);
                REQUIRE_NOTHROW(subdevice.open({ modes[i] }));
                REQUIRE_NOTHROW(subdevice.start([](frame fref) {}));
                REQUIRE_NOTHROW(subdevice.stop());
                REQUIRE_NOTHROW(subdevice.close());
            }
        }
    }
}

/* Apply heuristic test to check metadata attributes for sanity*/
void metadata_verification(const std::vector<frame_additional_data>& data)
{
    // Heuristics that we use to verify metadata
    // Metadata sanity
    // Frame numbers and timestamps increase monotonically
    // Sensor timestamp should be less or equal to frame timestamp
    // Exposure time and gain values are greater than zero
    // Sensor framerate is bounded to >0 and < 200 fps for uvc streams
    int64_t last_val[3] = { -1, -1, -1 };

    for (size_t i = 0; i < data.size(); i++)
    {

        // Check that Frame/Sensor timetamps, frame number rise monotonically
        for (int j = RS2_FRAME_METADATA_FRAME_COUNTER; j <= RS2_FRAME_METADATA_SENSOR_TIMESTAMP; j++)
        {
            if (data[i].frame_md.md_attributes[j].first)
            {
                int64_t value = data[i].frame_md.md_attributes[j].second;
                CAPTURE(value);
                CAPTURE(last_val[j]);

                REQUIRE_NOTHROW((value > last_val[0]));
                if (RS2_FRAME_METADATA_FRAME_COUNTER == j) // In addition, there shall be no frame number gaps
                {
                    REQUIRE_NOTHROW((1 == (value - last_val[j])));
                }

                last_val[j] = data[i].frame_md.md_attributes[j].second;
            }
        }

        // Sensor timestamp should be less or equal to frame timestamp
        if (data[i].frame_md.md_attributes[RS2_FRAME_METADATA_SENSOR_TIMESTAMP].first &&
            data[i].frame_md.md_attributes[RS2_FRAME_METADATA_FRAME_TIMESTAMP].first)
        {
            REQUIRE(data[i].frame_md.md_attributes[RS2_FRAME_METADATA_FRAME_TIMESTAMP].second >
                data[i].frame_md.md_attributes[RS2_FRAME_METADATA_SENSOR_TIMESTAMP].second);
        }

        // Exposure time and gain values are greater than zero
        if (data[i].frame_md.md_attributes[RS2_FRAME_METADATA_ACTUAL_EXPOSURE].first)
            REQUIRE(data[i].frame_md.md_attributes[RS2_FRAME_METADATA_ACTUAL_EXPOSURE].second > 0);
        if (data[i].frame_md.md_attributes[RS2_FRAME_METADATA_GAIN_LEVEL].first)
            REQUIRE(data[i].frame_md.md_attributes[RS2_FRAME_METADATA_GAIN_LEVEL].second > 0);
    }
}

TEST_CASE("Per-frame metadata sanity check", "[live]") {
    //Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        const int frames_before_start_measure = 130;
        const int frames_for_fps_measure = 100;
        const double msec_to_sec = 0.001;
        const int num_of_profiles_for_each_subdevice = 2;
        const float max_diff_between_real_and_metadata_fps = 0.2f;

        for (auto && subdevice : list) {
            std::vector<stream_profile> modes;
            REQUIRE_NOTHROW(modes = subdevice.get_stream_modes());

            REQUIRE(modes.size() > 0);
            WARN(subdevice.get_info(RS2_CAMERA_INFO_NAME));

            //the test will be done only on sub set of profile for each sub device
            for (int i = 0; i < modes.size(); i += static_cast<int>(std::ceil((float)modes.size() / (float)num_of_profiles_for_each_subdevice)))
            {
                if ((modes[i].width == 1920) ||                                                         // Full-HD is often times too heavy for the build machine to handle
                    ((RS2_STREAM_GPIO1 <= modes[i].stream) && (RS2_STREAM_GPIO4 >= modes[i].stream)))   // GPIO Requires external triggers to produce events
                    continue;   // Disabling for now

                CAPTURE(modes[i].format);
                CAPTURE(modes[i].fps);
                CAPTURE(modes[i].height);
                CAPTURE(modes[i].width);
                CAPTURE(modes[i].stream);

                std::vector<frame_additional_data> frames_additional_data;
                auto frames = 0;
                double start;
                std::condition_variable cv;
                std::mutex m;
                auto first = true;

                REQUIRE_NOTHROW(subdevice.open({ modes[i] }));
                disable_sensitive_options_for(subdevice);

                REQUIRE_NOTHROW(subdevice.start([&](frame f)
                {
                    if ((frames >= frames_before_start_measure) && (frames_additional_data.size() < frames_for_fps_measure))
                    {
                        if (first)
                        {
                            start = ctx.get_time();
                        }
                        first = false;

                        frame_additional_data data{ f.get_timestamp(),
                                                     f.get_frame_number(),
                                                     f.get_frame_timestamp_domain(),
                                                     f.get_stream_type(),
                                                     f.get_format() };

                        // Store frame metadata attributes, verify API behavior correctness
                        for (auto i = 0; i < RS2_FRAME_METADATA_COUNT; i++)
                        {
                            bool supported = false;
                            REQUIRE_NOTHROW(supported = f.supports_frame_metadata((rs2_frame_metadata)i));
                            if (supported)
                            {
                                rs2_metadata_t val{};
                                REQUIRE_NOTHROW(val = f.get_frame_metadata((rs2_frame_metadata)i));
                                data.frame_md.md_attributes[i] = std::make_pair(true, val);
                            }
                            else
                            {
                                REQUIRE_THROWS(f.get_frame_metadata((rs2_frame_metadata)i));
                                data.frame_md.md_attributes[i].first = false;
                            }
                        }


                        std::unique_lock<std::mutex> lock(m);
                        frames_additional_data.push_back(data);
                    }
                    frames++;
                    if (frames_additional_data.size() >= frames_for_fps_measure)
                    {
                        cv.notify_one();
                    }
                }));


                std::unique_lock<std::mutex> lock(m);
                REQUIRE(cv.wait_for(lock, std::chrono::seconds(30), [&] {return frames_additional_data.size() >= frames_for_fps_measure; }));

                REQUIRE_NOTHROW(subdevice.stop());
                REQUIRE_NOTHROW(subdevice.close());

                auto end = ctx.get_time();
                lock.unlock();

                auto seconds = (end - start)*msec_to_sec;

                CAPTURE(start);
                CAPTURE(end);

                if (seconds <= 0)
                {
                    std::cout << "Start " << std::fixed << start << "\n";
                    std::cout << "End   " << std::fixed << end << "\n";
                }

                REQUIRE(seconds > 0);

                auto actual_fps = (double)frames_additional_data.size() / (double)seconds;

                double metadata_seconds = frames_additional_data[frames_additional_data.size() - 1].timestamp - frames_additional_data[0].timestamp;
                metadata_seconds *= msec_to_sec;


                if (metadata_seconds <= 0)
                {
                    std::cout << "Start metadata " << std::fixed << frames_additional_data[0].timestamp << "\n";
                    std::cout << "End metadata   " << std::fixed << frames_additional_data[frames_additional_data.size() - 1].timestamp << "\n";
                }
                REQUIRE(metadata_seconds > 0);

                auto metadata_frames = frames_additional_data[frames_additional_data.size() - 1].frame_number - frames_additional_data[0].frame_number;
                auto metadata_fps = (double)metadata_frames / (double)metadata_seconds;

                for (auto i = 0; i < frames_additional_data.size() - 1; i++)
                {
                    CAPTURE(i);
                    CAPTURE(frames_additional_data[i].timestamp_domain);
                    CAPTURE(frames_additional_data[i + 1].timestamp_domain);
                    REQUIRE((frames_additional_data[i].timestamp_domain == frames_additional_data[i + 1].timestamp_domain));

                    CAPTURE(frames_additional_data[i].frame_number);
                    CAPTURE(frames_additional_data[i + 1].frame_number);

                    REQUIRE((frames_additional_data[i].frame_number < frames_additional_data[i + 1].frame_number));
                }
                CAPTURE(actual_fps);
                CAPTURE(metadata_fps);

                //it the diff in percentage between metadata fps and actual fps is bigger than max_diff_between_real_and_metadata_fps
                //the test will fail
                REQUIRE(std::abs(metadata_fps / actual_fps - 1) < max_diff_between_real_and_metadata_fps);

                // Verify per-frame metadata attributes
                metadata_verification(frames_additional_data);

                std::cout << modes[i].format << "MODE: " << modes[i].fps << " " << modes[i].height << " " << modes[i].width << " " << modes[i].stream << " succeed\n";
            }
        }
    }
}

void triger_error(const device& dev, int num)
{
    std::vector<uint8_t> raw_data(24, 0);
    raw_data[0] = 0x14;
    raw_data[2] = 0xab;
    raw_data[3] = 0xcd;
    raw_data[4] = 0x4d;
    raw_data[8] = num;
    if (auto debug = dev.as<debug_protocol>())
        debug.send_and_receive_raw_data(raw_data);
}

TEST_CASE("Error handling sanity", "[live]") {

    //Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        const int num_of_errors = 4;

        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        std::string notification_description;
        rs2_log_severity severity;
        std::condition_variable cv;
        std::mutex m;

        std::map<int, std::string> notification_descriptions;

        notification_descriptions[1] = "Hot laser power reduce";
        notification_descriptions[2] = "Hot laser disable";
        notification_descriptions[3] = "Flag B laser disable";

        //enable error polling
        for (auto && subdevice : list) {
            // disable_sensitive_options_for(subdevice);

            for (auto i = 1; i < num_of_errors; i++)
            {
                if (subdevice.supports(RS2_OPTION_ERROR_POLLING_ENABLED))
                {
                    disable_sensitive_options_for(subdevice);
                    REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_ERROR_POLLING_ENABLED, 1));
                    subdevice.set_notifications_callback([&](notification n)
                    {

                        std::unique_lock<std::mutex> lock(m);

                        notification_description = n.get_description();
                        severity = n.get_severity();
                        cv.notify_one();
                    });

                    triger_error(ctx.get_sensor_parent(subdevice), i);
                    std::unique_lock<std::mutex> lock(m);
                    CAPTURE(notification_description);
                    CAPTURE(severity);
                    CAPTURE(i);
                    auto pred = [&]() {
                        return notification_description.compare(notification_descriptions[i]) == 0
                            && severity == RS2_LOG_SEVERITY_ERROR;
                    };
                    REQUIRE(cv.wait_for(lock, std::chrono::seconds(10), pred));


                }
            }
        }


        //disable error polling

        auto got_error = false;
        for (auto && subdevice : list) {
            // disable_sensitive_options_for(subdevice);

            for (auto i = 1; i < 4; i++)
            {
                if (subdevice.supports(RS2_OPTION_ERROR_POLLING_ENABLED))
                {
                    disable_sensitive_options_for(subdevice);
                    REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_ERROR_POLLING_ENABLED, 0));
                    subdevice.set_notifications_callback([&](notification n)
                    {
                        got_error = true;
                        notification_description = n.get_description();

                    });

                    triger_error(ctx.get_sensor_parent(subdevice), i);
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    CAPTURE(notification_description);
                    REQUIRE(got_error == false);

                }
            }
        }
    }
}

TEST_CASE("Auto disabling control behavior", "[live]") {
    std::string SR300_PID = "0x0aa5";
    std::stringstream s;
    s << SR300_PID;
    int sr300_pid;
    s >> std::hex >> sr300_pid;

    //Require at least one device to be plugged in
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        for (auto && subdevice : list)
        {
            if (!subdevice.supports(RS2_CAMERA_INFO_PRODUCT_ID))
            {
                continue;
            }
            std::string pid = subdevice.get_info(RS2_CAMERA_INFO_PRODUCT_ID);
            std::stringstream s;
            s << pid;
            int curr_pid;
            s >> std::hex >> curr_pid;

            auto info = subdevice.get_info(RS2_CAMERA_INFO_NAME);
            CAPTURE(info);

            option_range range{};
            float val{};
            if (curr_pid != sr300_pid && subdevice.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
            {
                SECTION("Disable auto exposure when setting a value")
                {
                    REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 1));
                    REQUIRE_NOTHROW(range = subdevice.get_option_range(RS2_OPTION_EXPOSURE));
                    REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_EXPOSURE, range.max));
                    CAPTURE(range.max);
                    REQUIRE_NOTHROW(val = subdevice.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE));
                    REQUIRE(val == 0);
                }
            }

            if (subdevice.supports(RS2_OPTION_EMITTER_ENABLED))
            {
                SECTION("Disable emitter when setting a value")
                {
                    for (auto elem : {0, 2})
                    {
                        REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_EMITTER_ENABLED, elem));
                        REQUIRE_NOTHROW(range = subdevice.get_option_range(RS2_OPTION_LASER_POWER));
                        REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_LASER_POWER, range.max));
                        CAPTURE(range.max);
                        REQUIRE_NOTHROW(val = subdevice.get_option(RS2_OPTION_EMITTER_ENABLED));
                        REQUIRE(val == 1);
                    }
                }
            }

            if (subdevice.supports(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE))
            {
                SECTION("Disable white balance when setting a value")
                {
                    if (subdevice.supports(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE) && subdevice.supports(RS2_OPTION_WHITE_BALANCE))
                    {
                        REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, 1));
                        REQUIRE_NOTHROW(range = subdevice.get_option_range(RS2_OPTION_WHITE_BALANCE));
                        REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_WHITE_BALANCE, range.max));
                        CAPTURE(range.max);
                        REQUIRE_NOTHROW(val = subdevice.get_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE));
                        REQUIRE(val == 0);
                    }
                }
            }
        }
    }
}

TEST_CASE("Advanced Mode controls", "[live]") {
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        device_list list;
        REQUIRE_NOTHROW(list = ctx.query_devices());
        REQUIRE(list.size() > 0);

        auto dev = list.front();

        auto info = dev.get_info(RS2_CAMERA_INFO_NAME);
        CAPTURE(info);
        if (dev.is<rs400::advanced_mode>())
        {
            rs400::advanced_mode advanced = dev.as<rs400::advanced_mode>();
            REQUIRE(advanced.is_enabled());

            {
            STDepthControlGroup ctrl_curr{};
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_depth_control(0));
            STDepthControlGroup ctrl_min{};
            REQUIRE_NOTHROW(ctrl_min = advanced.get_depth_control(1));
            STDepthControlGroup ctrl_max{};
            REQUIRE_NOTHROW(ctrl_max = advanced.get_depth_control(2));
            REQUIRE_NOTHROW(advanced.set_depth_control(ctrl_min));
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_depth_control(0));
            REQUIRE(ctrl_curr == ctrl_min);
            }

            {
            STRsm ctrl_curr{};
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_rsm(0));
            STRsm ctrl_min{};
            REQUIRE_NOTHROW(ctrl_min = advanced.get_rsm(1));
            STRsm ctrl_max{};
            REQUIRE_NOTHROW(ctrl_max = advanced.get_rsm(2));
            REQUIRE_NOTHROW(advanced.set_rsm(ctrl_min));
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_rsm(0));
            REQUIRE(ctrl_curr == ctrl_min);
            }

            {
            STRauSupportVectorControl ctrl_curr{};
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_rau_support_vector_control(0));
            STRauSupportVectorControl ctrl_min{};
            REQUIRE_NOTHROW(ctrl_min = advanced.get_rau_support_vector_control(1));
            STRauSupportVectorControl ctrl_max{};
            REQUIRE_NOTHROW(ctrl_max = advanced.get_rau_support_vector_control(2));
            REQUIRE_NOTHROW(advanced.set_rau_support_vector_control(ctrl_min));
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_rau_support_vector_control(0));
            REQUIRE(ctrl_curr == ctrl_min);
            }

            {
            STColorControl ctrl_curr{};
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_color_control(0));
            STColorControl ctrl_min{};
            REQUIRE_NOTHROW(ctrl_min = advanced.get_color_control(1));
            STColorControl ctrl_max{};
            REQUIRE_NOTHROW(ctrl_max = advanced.get_color_control(2));
            REQUIRE_NOTHROW(advanced.set_color_control(ctrl_min));
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_color_control(0));
            REQUIRE(ctrl_curr == ctrl_min);
            }

            {
            STRauColorThresholdsControl ctrl_curr{};
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_rau_thresholds_control(0));
            STRauColorThresholdsControl ctrl_min{};
            REQUIRE_NOTHROW(ctrl_min = advanced.get_rau_thresholds_control(1));
            STRauColorThresholdsControl ctrl_max{};
            REQUIRE_NOTHROW(ctrl_max = advanced.get_rau_thresholds_control(2));
            REQUIRE_NOTHROW(advanced.set_rau_thresholds_control(ctrl_min));
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_rau_thresholds_control(0));
            REQUIRE(ctrl_curr == ctrl_min);
            }

            {
            STSloColorThresholdsControl ctrl_curr{};
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_slo_color_thresholds_control(0));
            STSloColorThresholdsControl ctrl_min{};
            REQUIRE_NOTHROW(ctrl_min = advanced.get_slo_color_thresholds_control(1));
            STSloColorThresholdsControl ctrl_max{};
            REQUIRE_NOTHROW(ctrl_max = advanced.get_slo_color_thresholds_control(2));
            REQUIRE_NOTHROW(advanced.set_slo_color_thresholds_control(ctrl_min));
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_slo_color_thresholds_control(0));
            REQUIRE(ctrl_curr == ctrl_min);
            }

            {
            STSloPenaltyControl ctrl_curr{};
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_slo_penalty_control(0));
            STSloPenaltyControl ctrl_min{};
            REQUIRE_NOTHROW(ctrl_min = advanced.get_slo_penalty_control(1));
            STSloPenaltyControl ctrl_max{};
            REQUIRE_NOTHROW(ctrl_max = advanced.get_slo_penalty_control(2));
            REQUIRE_NOTHROW(advanced.set_slo_penalty_control(ctrl_min));
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_slo_penalty_control(0));
            REQUIRE(ctrl_curr == ctrl_min);
            }

            {
            STHdad ctrl_curr{};
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_hdad(0));
            STHdad ctrl_min{};
            REQUIRE_NOTHROW(ctrl_min = advanced.get_hdad(1));
            STHdad ctrl_max{};
            REQUIRE_NOTHROW(ctrl_max = advanced.get_hdad(2));
            REQUIRE_NOTHROW(advanced.set_hdad(ctrl_min));
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_hdad(0));
            REQUIRE(ctrl_curr == ctrl_min);
            }

            {
            STColorCorrection ctrl_curr{};
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_color_correction(0));
            STColorCorrection ctrl_min{};
            REQUIRE_NOTHROW(ctrl_min = advanced.get_color_correction(1));
            STColorCorrection ctrl_max{};
            REQUIRE_NOTHROW(ctrl_max = advanced.get_color_correction(2));
            REQUIRE_NOTHROW(advanced.set_color_correction(ctrl_min));
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_color_correction(0));
            REQUIRE(ctrl_curr == ctrl_min);
            }

            {
            STAEControl ctrl_curr{};
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_ae_control(0));
            STAEControl ctrl_min{};
            REQUIRE_NOTHROW(ctrl_min = advanced.get_ae_control(1));
            STAEControl ctrl_max{};
            REQUIRE_NOTHROW(ctrl_max = advanced.get_ae_control(2));
            REQUIRE_NOTHROW(advanced.set_ae_control(ctrl_min));
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_ae_control(0));
            REQUIRE(ctrl_curr == ctrl_min);
            }

            {
            STDepthTableControl ctrl_curr{};
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_depth_table(0));
            STDepthTableControl ctrl_min{};
            REQUIRE_NOTHROW(ctrl_min = advanced.get_depth_table(1));
            STDepthTableControl ctrl_max{};
            REQUIRE_NOTHROW(ctrl_max = advanced.get_depth_table(2));
            REQUIRE_NOTHROW(advanced.set_depth_table(ctrl_min));
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_depth_table(0));
            REQUIRE(ctrl_curr == ctrl_min);
            }

            {
            STCensusRadius ctrl_curr{};
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_census(0));
            STCensusRadius ctrl_min{};
            REQUIRE_NOTHROW(ctrl_min = advanced.get_census(1));
            STCensusRadius ctrl_max{};
            REQUIRE_NOTHROW(ctrl_max = advanced.get_census(2));
            REQUIRE_NOTHROW(advanced.set_census(ctrl_min));
            REQUIRE_NOTHROW(ctrl_curr = advanced.get_census(0));
            REQUIRE(ctrl_curr == ctrl_min);
            }
        }
    }
}

std::pair<std::shared_ptr<device>, std::weak_ptr<device>> make_device(device_list& list)
{
    REQUIRE(list.size() > 0);

    std::shared_ptr<device> dev;
    REQUIRE_NOTHROW(dev = std::make_shared<device>(list[0]));
    std::weak_ptr<device> weak_dev(dev);

    disable_sensitive_options_for(*dev);

    return std::pair<std::shared_ptr<device>, std::weak_ptr<device>>(dev, weak_dev);
}

void reset_device(std::shared_ptr<device>& strong, std::weak_ptr<device>& weak, device_list& list, const device& new_dev)
{
    strong.reset();
    weak.reset();
    list = nullptr;
    strong = std::make_shared<device>(new_dev);
    weak = strong;
    disable_sensitive_options_for(*strong);
}

TEST_CASE("Disconnect events works", "[live]") {

    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        device_list list;
        REQUIRE_NOTHROW(list = ctx.query_devices());

        auto dev = make_device(list);
        auto dev_strong = dev.first;
        auto dev_weak = dev.second;


        auto disconnected = false;
        auto connected = false;

        std::string serial;

        REQUIRE_NOTHROW(serial = dev_strong->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));

        std::condition_variable cv;
        std::mutex m;

        //Setting up devices change callback to notify the test about device disconnection
        REQUIRE_NOTHROW(ctx.set_devices_changed_callback([&, dev_weak](event_information& info) mutable
        {
            auto&& strong = dev_weak.lock();
            {
                if (strong)
                {
                    if (info.was_removed(*strong))
                    {
                        std::unique_lock<std::mutex> lock(m);
                        disconnected = true;
                        cv.notify_one();
                    }


                    for (auto d : info.get_new_devices())
                    {
                        for (auto&& s : d.query_sensors())
                            disable_sensitive_options_for(s);

                        if (serial == d.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER))
                        {
                            try
                            {
                                std::unique_lock<std::mutex> lock(m);
                                connected = true;
                                cv.notify_one();
                                break;
                            }
                            catch (...)
                            {

                            }
                        }
                    }
                }

            }}));
        //forcing hardware reset to simulate device disconnection
        dev_strong->hardware_reset();

        {
            std::unique_lock<std::mutex> lock(m);
            //Check that device disconnection is registered within reasonable period of time.
            REQUIRE(cv.wait_for(lock, std::chrono::seconds(10), [&]() { return disconnected; }));
        }

        //Check that after the library reported device disconnection, operations on old device object will return error
        REQUIRE_THROWS(dev_strong->query_sensors().front().close());

        {
            std::unique_lock<std::mutex> lock(m);
            //Check that after reset the device gets connected back within reasonable period of time
            REQUIRE(cv.wait_for(lock, std::chrono::seconds(10), [&]() { return connected; }));
        }
    }
}

TEST_CASE("Connect events works", "[live]") {

    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        device_list list;
        REQUIRE_NOTHROW(list = ctx.query_devices());

        auto dev = make_device(list);
        auto dev_strong = dev.first;
        auto dev_weak = dev.second;

        std::string serial;

        REQUIRE_NOTHROW(serial = dev_strong->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));

        auto disconnected = false;
        auto connected = false;
        std::condition_variable cv;
        std::mutex m;

        //Setting up devices change callback to notify the test about device disconnection and connection
        REQUIRE_NOTHROW(ctx.set_devices_changed_callback([&, dev_weak](event_information& info) mutable
        {
            auto&& strong = dev_weak.lock();
            {
                if (strong)
                {
                    if (info.was_removed(*strong))
                    {
                        std::unique_lock<std::mutex> lock(m);
                        disconnected = true;
                        cv.notify_one();
                    }


                    for (auto d : info.get_new_devices())
                    {
                        if (serial == d.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER))
                        {
                            try
                            {
                                std::unique_lock<std::mutex> lock(m);

                                reset_device(dev_strong, dev_weak, list, d);

                                connected = true;
                                cv.notify_one();
                                break;
                            }
                            catch (...)
                            {

                            }
                        }
                    }
                }

            }}));

        //forcing hardware reset to simulate device disconnection
        dev_strong->hardware_reset();

        std::unique_lock<std::mutex> lock(m);
        //Check that device disconnection is registered within reasonable period of time.
        REQUIRE(cv.wait_for(lock, std::chrono::seconds(10), [&]() {return disconnected; }));
        //Check that after reset the device gets connected back within reasonable period of time
        REQUIRE(cv.wait_for(lock, std::chrono::seconds(10), [&]() {return connected; }));
    }
}

std::shared_ptr<std::function<void(frame fref)>> check_stream_sanity(const context& ctx, const sensor& dev, int num_of_frames, bool infinite = false)
{
    std::shared_ptr<std::condition_variable> cv = std::make_shared<std::condition_variable>();
    std::shared_ptr<std::mutex> m = std::make_shared<std::mutex>();
    std::shared_ptr<std::map<rs2_stream, int>> streams_frames = std::make_shared<std::map<rs2_stream, int>>();
    std::vector<sensor> devs;

    std::shared_ptr<std::function<void(frame fref)>>  func;


    std::vector<stream_profile> profiles = {
    {RS2_STREAM_DEPTH, 640, 480, 60, RS2_FORMAT_Z16},
    {RS2_STREAM_FISHEYE, 640, 480, 60, RS2_FORMAT_RAW8}};

    for (auto sub : ctx.get_sensor_parent(dev).query_sensors())
    {
        std::vector<stream_profile> modes;
        REQUIRE_NOTHROW(modes = sub.get_stream_modes());

        for(auto p:profiles )
        {
            if(std::find_if(modes.begin(), modes.end(), [&](stream_profile profile){return p==profile;}) < modes.end())
            {

                (*streams_frames)[p.stream] = 0;

                REQUIRE_NOTHROW(sub.open(p));
                devs.push_back(sub);

                func = std::make_shared< std::function<void(frame fref)>>([devs, num_of_frames, m, streams_frames, cv](frame fref) mutable
                    {
                        std::unique_lock<std::mutex> lock(*m);
                        auto stream = fref.get_stream_type();
                        streams_frames->at(stream)++;
                        if(streams_frames->at(stream) >= num_of_frames)
                            cv->notify_one();

                    });
                REQUIRE_NOTHROW(sub.start(*func));
                break;
            }
        }

    }

    REQUIRE(streams_frames->size()>0);

    std::unique_lock<std::mutex> lock(*m);
    cv->wait_for(lock, std::chrono::seconds(30), [&]
    {
        for (auto f : (*streams_frames))
        {
            if (f.second < num_of_frames)
                return false;
        }
        return true;
    });

    if (!infinite)
    {
        for(auto dev:devs)
        {
            REQUIRE_NOTHROW(dev.stop());
            REQUIRE_NOTHROW(dev.close());
        }
    }

    return func;
}



TEST_CASE("Connect Disconnect events while streaming", "[live]") {
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        device_list list;
        REQUIRE_NOTHROW(list = ctx.query_devices());


        std::string serial;

        auto dev = make_device(list);
        auto dev_strong = dev.first;
        auto dev_weak = dev.second;


        REQUIRE_NOTHROW(serial = dev_strong->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));


        auto disconnected = false;
        auto connected = false;
        std::condition_variable cv;
        std::mutex m;

        //Setting up devices change callback to notify the test about device disconnection and connection
        REQUIRE_NOTHROW(ctx.set_devices_changed_callback([&, dev_weak](event_information& info) mutable
        {
            auto&& strong = dev_weak.lock();
            {
                if (strong)
                {
                    if (info.was_removed(*strong))
                    {
                        std::unique_lock<std::mutex> lock(m);
                        disconnected = true;
                        cv.notify_one();
                    }


                    for (auto d : info.get_new_devices())
                    {
                        if (serial == d.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER))
                        {
                            try
                            {
                                std::unique_lock<std::mutex> lock(m);

                                reset_device(dev_strong, dev_weak, list, d);

                                connected = true;
                                cv.notify_one();
                                break;
                            }
                            catch (...)
                            {

                            }
                        }
                    }
                }

            }}));

        for (auto&& s : dev_strong->query_sensors())
            auto func = check_stream_sanity(ctx, s, 1, true);

        for (auto i = 0; i < 3; i++)
        {
            //forcing hardware reset to simulate device disconnection
            dev_strong->hardware_reset();

            {
                std::unique_lock<std::mutex> lock(m);
                //Check that device disconnection is registered within reasonable period of time.
                REQUIRE(cv.wait_for(lock, std::chrono::seconds(10), [&]() {return disconnected; }));
                //Check that after reset the device gets connected back within reasonable period of time
                REQUIRE(cv.wait_for(lock, std::chrono::seconds(10), [&]() {return connected; }));
            }

            for (auto&& s : dev_strong->query_sensors())
                auto func = check_stream_sanity(ctx, s, 10);

            disconnected = connected = false;
        }

    }
}

void check_controlls_sanity(const context& ctx, const sensor& dev)
{
    for (auto d : ctx.get_sensor_parent(dev).query_sensors())
    {
        for (auto i = 0; i < RS2_OPTION_COUNT; i++)
        {
            if (d.supports((rs2_option)i))
                REQUIRE_NOTHROW(d.get_option((rs2_option)i));
        }
    }
}

TEST_CASE("Connect Disconnect events while controlls", "[live]")
{
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        device_list list;
        REQUIRE_NOTHROW(list = ctx.query_devices());

        auto dev = make_device(list);
        auto dev_strong = dev.first;
        auto dev_weak = dev.second;

        std::string serial;

        REQUIRE_NOTHROW(serial = dev_strong->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));


        auto disconnected = false;
        auto connected = false;
        std::condition_variable cv;
        std::mutex m;

        //Setting up devices change callback to notify the test about device disconnection and connection
        REQUIRE_NOTHROW(ctx.set_devices_changed_callback([&, dev_weak](event_information& info) mutable
        {
            auto&& strong = dev_weak.lock();
            {
                if (strong)
                {
                    if (info.was_removed(*strong))
                    {
                        std::unique_lock<std::mutex> lock(m);
                        disconnected = true;
                        cv.notify_one();
                    }


                    for (auto d : info.get_new_devices())
                    {
                        if (serial == d.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER))
                        {
                            try
                            {
                                std::unique_lock<std::mutex> lock(m);

                                reset_device(dev_strong, dev_weak, list, d);

                                connected = true;
                                cv.notify_one();
                                break;
                            }
                            catch (...)
                            {

                            }
                        }
                    }
                }

            }}));

        //forcing hardware reset to simulate device disconnection
        dev_strong->hardware_reset();

        std::unique_lock<std::mutex> lock(m);
        //Check that device disconnection is registered within reasonable period of time.
        REQUIRE(cv.wait_for(lock, std::chrono::seconds(10), [&]() {return disconnected; }));
        //Check that after reset the device gets connected back within reasonable period of time
        REQUIRE(cv.wait_for(lock, std::chrono::seconds(10), [&]() {return connected; }));

        for (auto&& s : dev_strong->query_sensors())
            check_controlls_sanity(ctx, s);
    }

}

TEST_CASE("device_hub", "[live]") {

    rs2::context ctx;

    std::shared_ptr<device> dev;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        rs2::util::device_hub hub(ctx);
        REQUIRE_NOTHROW(dev = std::make_shared<device>(hub.wait_for_device()));

        std::weak_ptr<device> weak(dev);

        disable_sensitive_options_for(*dev);

        dev->hardware_reset();

        while(hub.is_connected(*dev))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        REQUIRE_NOTHROW(dev = std::make_shared<device>(hub.wait_for_device()));
        disable_sensitive_options_for(*dev);

        for (auto&& s : dev->query_sensors())
            check_controlls_sanity(ctx, s);
    }
}


class AC_Mock_Device
{
public:
    using SensorType = AC_Mock_Device;

    std::vector<SensorType> query_sensors() const
    {
        return std::vector<SensorType>(1, *this);
    }

    AC_Mock_Device(std::vector<stream_profile> modes, bool result) : result(result), modes(std::move(modes)), expected() {};
    void set_expected(std::vector<stream_profile> profiles) { expected = profiles; };

    std::vector<AC_Mock_Device> get_adjacent_devices() const { return{ *this }; }
    std::vector<stream_profile> get_stream_modes() const { return modes; };
    bool open(std::vector<stream_profile> profiles) const {
        for (auto & profile : profiles) {
            if (profile.has_wildcards()) return false;
            if (std::none_of(begin(expected), end(expected), [&profile](const stream_profile &p) {return p.match(profile); }))
                return false;
        }
        return true;
    }

    rs2_intrinsics get_intrinsics(stream_profile profile) const {
        return{ 0, 0, 0.0, 0.0, 0.0, 0.0, RS2_DISTORTION_COUNT,{ 0.0, 0.0, 0.0, 0.0, 0.0 } };
    }
    const char * get_camera_info(rs2_camera_info info) const {
        switch (info) {
        case RS2_CAMERA_INFO_NAME: return "Dummy Module";
        default: return "";
        }
    }

    std::string get_info(rs2_camera_info info) const { return "Mock"; }

    template<class T>
    void start(T callback) const
    {
        REQUIRE(result == true);
    }

    void stop() const
    {
        // TODO
    }

    void close() const
    {
        // TODO
    }

private:
    std::vector<stream_profile> modes, expected;
    bool result;
};

TEST_CASE("Auto-complete feature works", "[offline][util::config]") {
    // dummy device can provide the following profiles:
    AC_Mock_Device dev({ { RS2_STREAM_DEPTH   , 640, 240,  10, RS2_FORMAT_Z16 },
                      { RS2_STREAM_DEPTH   , 640, 240,  30, RS2_FORMAT_Z16 },
                      { RS2_STREAM_DEPTH   , 640, 240, 110, RS2_FORMAT_Z16 },
                      { RS2_STREAM_DEPTH   , 640, 480,  10, RS2_FORMAT_Z16 },
                      { RS2_STREAM_DEPTH   , 640, 480,  30, RS2_FORMAT_Z16 },
                      { RS2_STREAM_INFRARED, 640, 240,  10, RS2_FORMAT_Y8  },
                      { RS2_STREAM_INFRARED, 640, 240,  30, RS2_FORMAT_Y8  },
                      { RS2_STREAM_INFRARED, 640, 240, 110, RS2_FORMAT_Y8  },
                      { RS2_STREAM_INFRARED, 640, 480,  10, RS2_FORMAT_Y8  },
                      { RS2_STREAM_INFRARED, 640, 480,  30, RS2_FORMAT_Y8  },
                      { RS2_STREAM_INFRARED, 640, 480, 200, RS2_FORMAT_Y8  } }, true);
    util::Config<AC_Mock_Device> config;

    struct Test {
        std::vector<stream_profile> given,       // We give these profiles to the config class
            expected;    // pool of profiles the config class can return. Leave empty if auto-completer is expected to fail
    };
    std::vector<Test> tests = {
        // Test 0 (Depth always has RS2_FORMAT_Z16)
       { { { RS2_STREAM_DEPTH   ,   0,   0,   0, RS2_FORMAT_ANY } },   // given
         { { RS2_STREAM_DEPTH   ,   0,   0,   0, RS2_FORMAT_Z16 } } }, // expected
        // Test 1 (IR always has RS2_FORMAT_Y8)
       { { { RS2_STREAM_INFRARED,   0,   0,   0, RS2_FORMAT_ANY } },   // given
         { { RS2_STREAM_INFRARED,   0,   0,   0, RS2_FORMAT_Y8  } } }, // expected
        // Test 2 (No 200 fps depth)
       { { { RS2_STREAM_DEPTH   ,   0,   0, 200, RS2_FORMAT_ANY } },   // given
         { } },                                                      // expected
        // Test 3 (Can request 200 fps IR)
       { { { RS2_STREAM_INFRARED,   0,   0, 200, RS2_FORMAT_ANY } },   // given
         { { RS2_STREAM_INFRARED,   0,   0, 200, RS2_FORMAT_ANY } } }, // expected
        // Test 4 (requesting IR@200fps + depth fails
       { { { RS2_STREAM_INFRARED,   0,   0, 200, RS2_FORMAT_ANY }, { RS2_STREAM_DEPTH   ,   0,   0,   0, RS2_FORMAT_ANY } },   // given
         { } },                                                                                                            // expected
        // Test 5 (Can't do 640x480@110fps a)
       { { { RS2_STREAM_INFRARED, 640, 480, 110, RS2_FORMAT_ANY } },   // given
         { } },                                                      // expected
        // Test 6 (Can't do 640x480@110fps b)
       { { { RS2_STREAM_DEPTH   , 640, 480,   0, RS2_FORMAT_ANY }, { RS2_STREAM_INFRARED,   0,   0, 110, RS2_FORMAT_ANY } },   // given
         { } },                                                                                                            // expected
        // Test 7 (Pull extra details from second stream a)
       { { { RS2_STREAM_DEPTH   , 640, 480,   0, RS2_FORMAT_ANY }, { RS2_STREAM_INFRARED,   0,   0,  30, RS2_FORMAT_ANY } },   // given
         { { RS2_STREAM_DEPTH   , 640, 480,  30, RS2_FORMAT_ANY }, { RS2_STREAM_INFRARED, 640, 480,  30, RS2_FORMAT_ANY } } }, // expected
        // Test 8 (Pull extra details from second stream b) [IR also supports 200, could fail if that gets selected]
       { { { RS2_STREAM_INFRARED, 640, 480,   0, RS2_FORMAT_ANY }, { RS2_STREAM_DEPTH   ,   0,   0,   0, RS2_FORMAT_ANY } },   // given
         { { RS2_STREAM_INFRARED, 640, 480,  10, RS2_FORMAT_ANY }, { RS2_STREAM_INFRARED, 640, 480,  30, RS2_FORMAT_ANY },     // expected - options for IR stream
           { RS2_STREAM_DEPTH   , 640, 480,  10, RS2_FORMAT_ANY }, { RS2_STREAM_DEPTH   , 640, 480,  30, RS2_FORMAT_ANY } } }  // expected - options for depth stream
    };

    for (int i = 0; i < tests.size(); ++i)
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
            util::Config<AC_Mock_Device>::multistream results;
            REQUIRE_NOTHROW(results = config.open(dev));
            //REQUIRE()s are in here
            results.start(0);
            results.stop();
            REQUIRE_NOTHROW(config.open(dev));
        }
    }
}
