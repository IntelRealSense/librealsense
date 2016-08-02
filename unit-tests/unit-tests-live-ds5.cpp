// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////
// This set of tests is valid only for the R200 camera //
/////////////////////////////////////////////////////////

#include <climits>
#include <sstream>

#define CATCH_CONFIG_MAIN

#include "unit-tests-live-ds-common.h"
#include "librealsense/rs.hpp"


TEST_CASE("DS5 devices support all required options", "[live] [DS-device]")
{
    // Require at least one device to be plugged in
    safe_context ctx;
    const int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count > 0);

    const int supported_options[] = { RS_OPTION_DS5_LASER_POWER };

    SECTION("DS5 supports specific extended UVC controls, and nothing else")
    {
        // For each device
        for (int i = 0; i<device_count; ++i)
        {
            rs_device * dev = rs_get_device(ctx, 0, require_no_error());
            REQUIRE(dev != nullptr);

            std::string name = dev->get_name();
            REQUIRE(std::string::npos != name.find("Intel RealSense DS5"));

            for (size_t i = 0; i<RS_OPTION_COUNT; ++i)
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

TEST_CASE("DS5 correctly recognizes invalid options", "[live] [DS-device]")
{
    rs::context ctx;
    REQUIRE(ctx.get_device_count() == 1);

    rs::device * dev = ctx.get_device(0);
    REQUIRE(nullptr!=dev);

    std::string name = dev->get_name();
    REQUIRE(std::string::npos != name.find("Intel RealSense DS5"));

    int index = 0;
    double val = 0;

    for (size_t i= 0; i <RS_OPTION_COUNT; i++)
    {
        index = i;
        rs::option opt = (rs::option)i;
        try
        {
            dev->get_options(&opt,1, &val);
        }
        catch(...)
        {
            REQUIRE(i==index); // Every invalid option queried must throw exception
        }

        try
        {
            dev->set_options(&opt,1, &val);
        }
        catch(...)
        {
            REQUIRE(i==index); // Each invoked option must throw exception
        }
    }
}


TEST_CASE("DS5 laser power control verification", "[live] [DS-device]")
{
    rs::context ctx;
    REQUIRE(ctx.get_device_count() == 1);

    rs::device * dev = ctx.get_device(0);
    REQUIRE(nullptr!=dev);

    std::string name = dev->get_name();
    REQUIRE(std::string::npos != name.find("Intel RealSense DS5"));

    int index = 0;
    double val = 0., res = 0.;

    double lsr_init_power = 0.;
    rs::option opt = rs::option::ds5_laser_power;

    dev->get_options(&opt,1, &lsr_init_power);
    INFO("Initial laser power value obtained from hardware is " << lsr_init_power);

    for (uint8_t i =0; i < 100; i++) // Laser power is specified in % of max power
    {
        val = i;
        try
        {
            dev->set_options(&opt,1, &val);
            dev->get_options(&opt,1, &res);
            CHECK(val==res);
        }
        catch(...)
        {
            WARN("Test failed on input value " << i);
            CHECK(false); // Each invoked option must throw exception
        }
    }
}
