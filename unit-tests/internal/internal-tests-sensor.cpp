// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "catch/catch.hpp"

#include <cmath>
#include <iostream>
#include <chrono>
#include <ctime>
#include <vector>
#include <algorithm>
#include <memory>

#include "../unit-tests-common.h"
#include "../unit-tests-expected.h"
#include "internal-tests-common.h"

#include "../src/sensor.h"
#include "../src/option.h"

using namespace librealsense;
using namespace librealsense::platform;

TEST_CASE("Sensor - Resolver Test", "[live]")
{
    // Check if the expected resolved profiles are equal to the resolver result

    auto devices = create_lrs_devices();
    for (auto&& device : devices)
    {
        auto pid = get_device_pid(*device);

        auto expected_resolver_set = generate_sensor_resolver_profiles(pid);

        for (auto&& expctd_rslvr_pairs : expected_resolver_set)
        {
            for (size_t i = 0; i < device->get_sensors_count(); i++)
            {
                auto&& snsr = device->get_sensor(i);
                auto syn_snsr = dynamic_cast<librealsense::synthetic_sensor*>(&snsr);
                auto snsr_name = snsr.get_info(RS2_CAMERA_INFO_NAME);
                auto&& expected_resolver_pairs = expected_resolver_set[snsr_name];

                // iterate over all of the color sensor expected resolver pairs
                for (auto&& expctd_rslvr_pr : expected_resolver_pairs)
                {
                    compare_profiles(*syn_snsr, expctd_rslvr_pr);
                }
            }
        }
    }
}

TEST_CASE("Sensor - Register Unregister Options", "[live]")
{
    // Register an option to the synthetic sensor and check if it applied to both synthetic sensor and raw sensor

    auto device = (create_lrs_devices())[0];
    auto&& snsr = device->get_sensor(0);
    auto syn_snsr = dynamic_cast<synthetic_sensor*>(&snsr);
    auto raw_snsr = syn_snsr->get_raw_sensor();

    // Register a new option
    rs2_option new_option_id = static_cast<rs2_option>(INT_MAX);
    auto new_option = std::make_shared<bool_option>();
    syn_snsr->register_option(new_option_id, new_option);

    // Verify that the option was registered both in the raw sensor and in the synthetic sensor
    REQUIRE(syn_snsr->supports_option(new_option_id) == true);
    REQUIRE(raw_snsr->supports_option(new_option_id) == true);

    // Unregister the option
    syn_snsr->unregister_option(new_option_id);

    // Verify that the option was is not supported any longer
    REQUIRE(syn_snsr->supports_option(new_option_id) == false);
    REQUIRE(raw_snsr->supports_option(new_option_id) == false);
}
