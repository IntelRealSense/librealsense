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
    auto list = ctx.query_devices();
    REQUIRE(list.size());
    auto dev1 = list.front();
    auto dev2 = list.front();
    std::array < device, 2> devices = { dev1, dev2 };

    rs2_option limits_toggle[2] = { RS2_OPTION_AUTO_EXPOSURE_LIMIT_TOGGLE, RS2_OPTION_AUTO_GAIN_LIMIT_TOGGLE };
    rs2_option limits_value[2] = { RS2_OPTION_AUTO_EXPOSURE_LIMIT, RS2_OPTION_AUTO_GAIN_LIMIT };
    std::array < std::map<rs2_option, sensor>, 2> picked_sensor;

    // 1. Scenario 1:
    //          - Change control value few times
    //          - Turn toggle off
    //          - Turn toggle on
    //          - Check that control limit value is the latest value
    // 2. Scenario 2:
    //          - Init 2 devices
    //          - Change control value for each device to different values
    //          - Toggle off each control
    //          - Toggle on each control
    //          - Check that control limit value in each device is set to the device cached value

    for (auto i = 0; i < 2; i++) // 2 controls
    {
        for (auto j = 0; j < 2; j++) // 2 devices
        {
            auto sensors = devices[j].query_sensors();
            for (auto& s : sensors)
            {
                std::string val = s.get_info(RS2_CAMERA_INFO_NAME);
                if (!s.supports(limits_value[i]))
                    continue;
                picked_sensor[j][limits_value[i]] = s.get();//std::make_shared<sensor>(s);
                auto range = s.get_option_range(limits_value[i]);
                // 1. Scenario 1:
                //    - Change control value few times
                //    - Turn toggle off
                //    - Turn toggle on
                //    - Check that control limit value is the latest value
                float values[3] = { range.min + 5.0f,  range.max / 4.0f, range.max - 5.0f };
                for (auto& val : values)
                    s.set_option(limits_value[i], val);
                s.set_option(limits_toggle[i], 0.0); // off
                s.set_option(limits_toggle[i], 1.0); // on
                auto limit = s.get_option(limits_value[i]);
                REQUIRE(limit == values[2]);
            }
        }
    }

        // 2. Scenario 2:
        //        - Init 2 devices
        //        - toggle on both dev1 and dev2 and set two distinct values for the auto-exposure /gain.
        //        - toggle both dev1and dev2 off.
        //        2.1. toggle dev1 on :
        //                  * verify that the limit value is the value that was stored(cached) in dev1.
        //                  * verify that for dev2 both the limitand the toggle values are similar to those of dev1
        //        2.2. toggle dev2 on :
        //                  * verify that the limit value is the value that was stored(cached) in dev2.

    for (auto i = 0; i < 2; i++) // exposure or gain
    {
        auto s1 = picked_sensor[0][limits_value[i]];
        auto s2 = picked_sensor[1][limits_value[i]];

        auto range = s1.get_option_range(limits_value[i]); // should be same range from both sensors
        s1.set_option(limits_value[i], range.max / 4.0f);
        s1.set_option(limits_toggle[i], 0.0); // off
        s2.set_option(limits_value[i], range.max - 5.0f);
        s2.set_option(limits_toggle[i], 0.0); // off

        // 2.1
        s1.set_option(limits_toggle[i], 1.0); // on
        auto limit1 = s1.get_option(limits_value[i]);
        REQUIRE(limit1 == range.max / 4.0);
        // keep toggle of dev2 off
        auto limit2 = s2.get_option(limits_value[i]);
        REQUIRE(limit1 == limit2);

        // 2.2
        s2.set_option(limits_toggle[i], 1.0); // on
        limit2 = s2.get_option(limits_value[i]);
        REQUIRE(limit2 == range.max - 5.0);
    }
}
