// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////
// This set of tests is valid only for the R200 camera //
/////////////////////////////////////////////////////////

#if !defined(MAKEFILE) || ( defined(LIVE_TEST) && defined(DS5_TEST) )

#include <climits>
#include <sstream>


#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include "unit-tests-live-ds-common.h"
#include "librealsense/rs.hpp"


TEST_CASE("RS400 device supports all required options", "[live] [DS-device]")
{
    rs::log_to_console(rs::log_severity::warn);
    // Require at least one device to be plugged in
    safe_context ctx;
    const int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count > 0);

    const int supported_options[] = { RS_OPTION_RS400_LASER_POWER };

    SECTION("DS5 supports specific extended UVC controls, and nothing else")
    {
        // For each device
        for (int i = 0; i < device_count; ++i)
        {
            rs_device * dev = rs_get_device(ctx, 0, require_no_error());
            REQUIRE(dev != nullptr);

            std::string name = dev->get_name();
            REQUIRE(std::string::npos != name.find("Intel RealSense DS5"));

            for (size_t i = 0; i < RS_OPTION_COUNT; ++i)
            {
                INFO("Checking support for " << (rs::option)i);
                if (std::find(std::begin(supported_options), std::end(supported_options), i) != std::end(supported_options))
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

TEST_CASE("RS400 correctly recognizes invalid options", "[live] [DS-device]")
{
    rs::context ctx;
    REQUIRE(ctx.get_device_count() == 1);

    rs::device * dev = ctx.get_device(0);
    REQUIRE(nullptr != dev);

    std::string name = dev->get_name();
    REQUIRE(std::string::npos != name.find("Intel RealSense RS400"));

    size_t index = 0;
    double val = 0;

    for (size_t i = 0; i < RS_OPTION_COUNT; i++)
    {
        index = i;
        rs::option opt = (rs::option)i;
        try
        {
            dev->get_options(&opt, 1, &val);
        }
        catch (...)
        {
            REQUIRE(i == index); // Every invalid option queried must throw exception
        }

        try
        {
            dev->set_options(&opt, 1, &val);
        }
        catch (...)
        {
            REQUIRE(i == index); // Each invoked option must throw exception
        }
    }
}

TEST_CASE("RS400 asynchronous controls", "[live] [DS-device]")
{
    for (int i=0; i< 10; i++)
    {
        INFO("CTX creation iteration " << i);

        rs::context ctx;
        REQUIRE(ctx.get_device_count() == 1);

        rs::device * dev = ctx.get_device(0);
        REQUIRE(nullptr != dev);

        std::string name = dev->get_name();
        INFO("Device name is " << name);
        REQUIRE(std::string::npos != name.find("Intel RealSense RS400"));

        double lsr_init_power = 0.;
        rs::option opt = rs::option::rs400_laser_power;

        dev->get_options(&opt, 1, &lsr_init_power);
        INFO("Initial laser power value obtained from hardware is " << lsr_init_power);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        int index = 0;
        double set_val = 1., reset_val = 0., res = 0.;

        for (uint8_t j = 0; j < 10; j++) // Laser power is specified in % of max power  - TODO: verify with FW
        {
            INFO("Set option iteration " << j);
            dev->set_options(&opt, 1, &set_val);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            dev->get_options(&opt, 1, &res);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            REQUIRE(set_val == res);

            dev->set_options(&opt, 1, &reset_val);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            dev->get_options(&opt, 1, &res);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            REQUIRE(reset_val == res);
        }

        dev->set_options(&opt, 1, &lsr_init_power);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

TEST_CASE("RS400 laser power control verification", "[live] [DS-device]")
{
    rs::context ctx;
    REQUIRE(ctx.get_device_count() == 1);

    rs::device * dev = ctx.get_device(0);
    REQUIRE(nullptr != dev);

    std::string name = dev->get_name();
    REQUIRE(std::string::npos != name.find("Intel RealSense RS400"));

    int index = 0;
    double set_val = 1., reset_val = 0., res = 0.;

    double lsr_init_power = 0.;
    rs::option opt = rs::option::rs400_laser_power;

    dev->get_options(&opt, 1, &lsr_init_power);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    INFO("Initial laser power value obtained from hardware is " << lsr_init_power);

    for (uint8_t i = 0; i < 10; i++) // Laser power is specified in % of max power  - TODO: verify with FW
    {
        dev->set_options(&opt, 1, &set_val);
        dev->get_options(&opt, 1, &res);
        REQUIRE(set_val == res);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        dev->set_options(&opt, 1, &reset_val);
        dev->get_options(&opt, 1, &res);
        REQUIRE(reset_val == res);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Revert to original value
    dev->set_options(&opt, 1, &lsr_init_power);
    dev->get_options(&opt, 1, &res);
    REQUIRE(lsr_init_power == res);
}

TEST_CASE("RS400 Streaming Formats validation", "[live] [DS-device]")
{
    // Require only one device to be plugged in
    safe_context ctx;
    const int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);

    std::string name = dev->get_name();
    REQUIRE(std::string::npos != name.find("Intel RealSense RS400"));

    SECTION( "Streaming depth configurations: [640X480] X [15,30,60]FPS ")
    {
        for ( auto fps : {15,30})
        {
            test_streaming(dev, {
                {RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, fps}
            });
        }
    }
}

TEST_CASE("RS400 Manual Gain Control verification", "[live] [DS-device]")
{
    rs::context ctx;
    REQUIRE(ctx.get_device_count() == 1);

    rs::device * dev = ctx.get_device(0);
    REQUIRE(nullptr != dev);

    std::string name = dev->get_name();
    REQUIRE(std::string::npos != name.find("Intel RealSense RS400"));

    int index = 0;
    double set_val = 30., reset_val = 80., res = 0.;

    double gain_ctrl_init = 0.;
    rs::option opt = rs::option::color_gain;

    dev->get_options(&opt, 1, &gain_ctrl_init);
    INFO("Initial gain control value obtained from hardware is " << gain_ctrl_init);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Apply several iterations
    for (uint8_t i = 0; i < 1; i++)
    {
        dev->set_options(&opt, 1, &set_val);
        dev->get_options(&opt, 1, &res);
        REQUIRE(set_val == res);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        dev->set_options(&opt, 1, &reset_val);
        dev->get_options(&opt, 1, &res);
        REQUIRE(reset_val == res);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Revert to original value
    dev->set_options(&opt, 1, &gain_ctrl_init);
    dev->get_options(&opt, 1, &res);
    REQUIRE(gain_ctrl_init == res);
}

TEST_CASE("RS400 Manual Exposure Control verification", "[live] [DS-device]")
{
    rs::context ctx;
    REQUIRE(ctx.get_device_count() == 1);

    rs::device * dev = ctx.get_device(0);
    REQUIRE(nullptr != dev);

    std::string name = dev->get_name();
    REQUIRE(std::string::npos != name.find("Intel RealSense RS400"));

    int index = 0;
    double set_val = 1200., reset_val = 80., res = 0.;

    double exposure_ctrl_init = 0.;
    rs::option opt = rs::option::r200_lr_exposure;

    dev->get_options(&opt, 1, &exposure_ctrl_init);
    INFO("Initial exposure control value obtained from hardware is " << exposure_ctrl_init);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Apply several iterations
    for (uint8_t i = 0; i < 1; i++)
    {
        dev->set_options(&opt, 1, &set_val);
        dev->get_options(&opt, 1, &res);
        REQUIRE(set_val == res);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        dev->set_options(&opt, 1, &reset_val);
        dev->get_options(&opt, 1, &res);
        REQUIRE(reset_val == res);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Revert to original value
    dev->set_options(&opt, 1, &exposure_ctrl_init);
    dev->get_options(&opt, 1, &res);
    REQUIRE(exposure_ctrl_init == res);
}

///////////////////////////////////
// Calibration information tests //
///////////////////////////////////

TEST_CASE("RS400 device extrinsics are within expected parameters", "[live] [sr300]")
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
            rs_get_device_extrinsics(dev, RS_STREAM_DEPTH, RS_STREAM_INFRARED2, &extrin, require_no_error());

            require_identity_matrix(extrin.rotation);
            require_zero_vector(extrin.translation);
        }
    }
}

#endif /* #if !defined(MAKEFILE) || ( defined(LIVE_TEST) && defined(DS5_TEST) ) */
