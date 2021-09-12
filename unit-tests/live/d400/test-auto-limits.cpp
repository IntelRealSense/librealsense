// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////////////////////////
// This set of tests is valid for any device that supports the HDR feature //
/////////////////////////////////////////////////////////////////////////////

//#test:device D400*

#include "../../catch.h"
#include "../../unit-tests-common.h"

using namespace rs2;

// HDR CONFIGURATION TESTS
TEST_CASE("Gain/ Exposure auto limits", "[live]") 
{
    rs2::context ctx;

    // Goal : test Gain/ Exposure limits in relation to enable/disable button
    // 1. check if initial values of limit control is set to max value
    // 2. enable then disable toggle to see if limit is still set on max value
    // 3. enable toggle, then change limit to lower value:
    //    - disable toggle : check if limit control is set to max value, then
    //    - enable toggle : check if control limit is set back to cached value

    auto list = ctx.query_devices();
    REQUIRE(list.size());
    auto dev = list.front();
    auto sensors = dev.query_sensors();

    rs2_option exposure_controls[2] = { RS2_OPTION_AUTO_EXPOSURE_LIMIT_TOGGLE , RS2_OPTION_AUTO_EXPOSURE_LIMIT };
    rs2_option gain_controls[2] = { RS2_OPTION_AUTO_GAIN_LIMIT_TOGGLE , RS2_OPTION_AUTO_GAIN_LIMIT };

    // Exposure
    auto limit_control = exposure_controls[1];
    auto enable_limit = exposure_controls[0];

    for (auto& s : sensors)
    {
        std::string val = s.get_info(RS2_CAMERA_INFO_NAME);
        if (!s.supports(limit_control))
            continue;
        auto range = s.get_option_range(limit_control);
        // 1. check if initial values of limit control is set to max value
        std::cout << "1. Check if initial values of limit control is set to max value" << std::endl;
        auto init_limit = s.get_option(limit_control);
        //auto enable = s.get_option(enable_limit);
        REQUIRE(init_limit == range.max);

        // 2. enable then disable toggle to see if limit is still set on max value
        std::cout << "2. Checking max limit .." << std::endl;
        //    2.1 enable control and check if value doesn't change
        std::cout << "  2.1 Enable control and check if value doesn't change" << std::endl;
        auto pre_enable = s.get_option(limit_control);
        s.set_option(enable_limit, 1.f);
        auto post_enable = s.get_option(limit_control);
        REQUIRE(post_enable == pre_enable);
        //    2.2 after #2.1, disable control and check if limit control value still set on max value
        std::cout << "  2.2 Disable control and check if limit control value still set on max value" << std::endl;
        s.set_option(enable_limit, 0.f);
        auto post_disable = s.get_option(limit_control);
        REQUIRE(post_disable == range.max);

        // 3. enable toggle, then change limit to lower value:
        std::cout << "3. Enable toggle, then change limit to lower value" << std::endl;
        float cache_value = range.min + range.max / 2.0;
        s.set_option(enable_limit, 1.f);
        s.set_option(limit_control, cache_value);
        //    3.1 disable control : check if limit control is set to max value, then
        std::cout << "  3.1 Disable control : check if limit control is set to max value" << std::endl;
        s.set_option(enable_limit, 0.f);
        post_disable = s.get_option(limit_control);
        //    3.2 enable toggle : check if control limit is set back to cached value
        std::cout << "  3.2 Enable toggle : check if control limit is set back to cached value" << std::endl;
        s.set_option(enable_limit, 1.f);
        post_enable = s.get_option(limit_control);
        REQUIRE(post_enable == cache_value);

     
    }
}
