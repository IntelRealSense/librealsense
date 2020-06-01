// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file f450-common.h

#include "f450-common.h"

void check_option_from_max_to_min_and_back(rs2::sensor sensor, rs2_option opt, int number_of_iterations, float tolerance);
void check_both_sensors_option_from_max_to_min_and_back(rs2::sensor sensor, rs2_option opt, rs2_option opt_second, int number_of_iterations, float tolerance);

TEST_CASE( "Exposure - each sensor and both - no streaming", "[F450]" ) {
    using namespace std;

    rs2::context ctx;

    auto devices_list = ctx.query_devices();
    size_t device_count = devices_list.size();
    REQUIRE(device_count > 0);
 
    std::vector<rs2::device> devices;

    for (auto i = 0u; i < device_count; i++)
    {
        try
        {
            auto dev = devices_list[i];
            devices.emplace_back(dev);
        }
        catch (...)
        {
            std::cout << "Failed to created device. Check SDK logs for details" << std::endl;
        }
    }

    rs2::sensor sensor = devices[0].first<rs2::fa_infrared_sensor>();

    int number_of_iterations = 20;
    float tolerance = 0.3; // for 30% - not absolute as done in gain
    
    //set AUTO EXPOSURE to manual
    sensor.set_option(RS2_OPTION_AUTO_EXPOSURE_MODE, 0);
    REQUIRE(sensor.get_option(RS2_OPTION_AUTO_EXPOSURE_MODE) == 0);

    rs2_option opt = RS2_OPTION_GAIN;

    check_option_from_max_to_min_and_back(sensor, RS2_OPTION_EXPOSURE, number_of_iterations, tolerance);
    //--------------------------------------------------------------------------
    check_option_from_max_to_min_and_back(sensor, RS2_OPTION_EXPOSURE_SECOND, number_of_iterations, tolerance);
    //--------------------------------------------------------------------------
    check_both_sensors_option_from_max_to_min_and_back(sensor, RS2_OPTION_EXPOSURE, RS2_OPTION_EXPOSURE_SECOND, number_of_iterations, tolerance);
    
}


void check_option_from_max_to_min_and_back(rs2::sensor sensor, rs2_option opt, int number_of_iterations, float tolerance)
{
    auto range = sensor.get_option_range(opt);
    std::cout << "-----------------------------------------------------------" << std::endl;
    std::cout << "Checking option = " << rs2_option_to_string(opt) << std::endl;

    //setting exposure to max
    float valueToSet = range.max;
    sensor.set_option(opt, valueToSet);
    float valueAfterChange = sensor.get_option(opt);
    int iterations = number_of_iterations;
    while (iterations-- && valueAfterChange != valueToSet)
    {
        valueAfterChange = sensor.get_option(opt);
    }
    bool result = (valueAfterChange >= (1.f - tolerance) * valueToSet) && (valueAfterChange <= (1.f + tolerance) * valueToSet);
    REQUIRE(result);
    std::cout << std::endl;
    std::cout << "Max Value Checked" << std::endl;

    //going down dividing by 2 at each iteration until min
    checkOption(sensor, opt, valueAfterChange, 15.0f/*range.min*/, number_of_iterations, tolerance, [](float value, float limit) {
        return value > limit;
        },
        [&](float& value) {
            value /= 2.f;
        },
            [tolerance](float valueAfterChange, float valueRequested) {
            return (valueAfterChange >= (1.f - tolerance) * valueRequested) && (valueAfterChange <= (1.f + tolerance)* valueRequested);
        }
        );

    valueAfterChange = sensor.get_option(opt);
    REQUIRE(valueAfterChange == 15.0f/*range.min*/);

    std::cout << std::endl;
    std::cout << "Option decremented and got to min value Checked" << std::endl;

    //then back to max multiplying by 2 until max
    checkOption(sensor, opt, valueAfterChange, range.max, number_of_iterations, tolerance, [](float value, float limit) {
        return value < limit;
        },
        [&](float& value) {
            value *= 2.f;
        },
            [tolerance](float valueAfterChange, float valueRequested) {
            return (valueAfterChange >= (1.f - tolerance) * valueRequested) && (valueAfterChange <= (1.f + tolerance) * valueRequested);
        });
    valueAfterChange = sensor.get_option(opt);
    REQUIRE(valueAfterChange == range.max);

    std::cout << std::endl;
    std::cout << "Option incremented and got back to max value Checked" << std::endl;
}


void check_both_sensors_option_from_max_to_min_and_back(rs2::sensor sensor, rs2_option opt, rs2_option opt_second, int number_of_iterations, float tolerance)
{
    std::cout << "-----------------------------------------------------------" << std::endl;
    std::cout << "Checking option = " << rs2_option_to_string(opt) << " for both sensors" <<std::endl;

    auto range = sensor.get_option_range(opt);
    auto range_second = sensor.get_option_range(opt_second);
    REQUIRE(range.min == range_second.min);
    REQUIRE(range.max == range_second.max);
    REQUIRE(range.step == range_second.step);
    std::cout << "Min, Max, Step checked same for both sensors" << std::endl;


    //setting exposure to max
    float valueToSet = range.max;
    sensor.set_option(opt, valueToSet);
    sensor.set_option(opt_second, valueToSet);
    float valueAfterChange = sensor.get_option(opt);
    float valueAfterChange_second = sensor.get_option(opt_second);
    int iterations = number_of_iterations;
    while (iterations-- && valueAfterChange != valueToSet)
    {
        valueAfterChange = sensor.get_option(opt);
    }
    iterations = number_of_iterations;
    while (iterations-- && valueAfterChange_second != valueToSet)
    {
        valueAfterChange_second = sensor.get_option(opt_second);
    }
    bool result = (valueAfterChange >= (1.f - tolerance) * valueToSet) && (valueAfterChange <= (1.f + tolerance) * valueToSet);
    REQUIRE(result);
    bool result_second = (valueAfterChange_second >= (1.f - tolerance) * valueToSet) && (valueAfterChange_second <= (1.f + tolerance) * valueToSet);
    REQUIRE(result_second);
    std::cout << std::endl;
    std::cout << "Max Value Checked" << std::endl;

    //going down dividing by 2 at each iteration until min
    checkOptionForBothSensors(sensor, opt, opt_second, valueAfterChange, 15.0f/*range.min*/, number_of_iterations, tolerance, [](float value, float limit) {
        return value > limit;
        },
        [&](float& value) {
            value /= 2.f;
        },
            [tolerance](float valueAfterChange, float valueRequested) {
            return (valueAfterChange >= (1.f - tolerance) * valueRequested) && (valueAfterChange <= (1.f + tolerance) * valueRequested);
        }
        );

    valueAfterChange = sensor.get_option(opt);
    valueAfterChange_second = sensor.get_option(opt_second);
    REQUIRE(valueAfterChange == 15.0f/*range.min*/);
    REQUIRE(valueAfterChange_second == 15.0f/*range.min*/);

    std::cout << std::endl;
    std::cout << "Option decremented and got to min value Checked" << std::endl;

    //then back to max multiplying by 2 until max
    checkOptionForBothSensors(sensor, opt, opt_second, valueAfterChange, range.max, number_of_iterations, tolerance, [](float value, float limit) {
        return value < limit;
        },
        [&](float& value) {
            value *= 2.f;
        },
            [tolerance](float valueAfterChange, float valueRequested) {
            return (valueAfterChange >= (1.f - tolerance) * valueRequested) && (valueAfterChange <= (1.f + tolerance) * valueRequested);
        }
        );
    valueAfterChange = sensor.get_option(opt);
    valueAfterChange_second = sensor.get_option(opt_second);
    REQUIRE(valueAfterChange == range.max);
    REQUIRE(valueAfterChange_second == range.max);

    std::cout << std::endl;
    std::cout << "Option incremented and got back to max value Checked" << std::endl;
}