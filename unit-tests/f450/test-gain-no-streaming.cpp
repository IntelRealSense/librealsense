// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file f450-common.h

#include "f450-common.h"

void check_option_from_min_to_max_and_back(rs2::sensor sensor, rs2_option opt, int number_of_iterations, float tolerance);
void check_both_sensors_option_from_min_to_max_and_back(rs2::sensor sensor, rs2_option opt, rs2_option opt_second, int number_of_iterations, float tolerance);

TEST_CASE( "Gain - each sensor and both - no streaming", "[F450]" ) {
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

    int number_of_iterations = 5;
    float tolerance = 2;
    //set AUTO EXPOSURE to manual
    sensor.set_option(RS2_OPTION_AUTO_EXPOSURE_MODE, 0);
    REQUIRE(sensor.get_option(RS2_OPTION_AUTO_EXPOSURE_MODE) == 0);

    rs2_option opt = RS2_OPTION_GAIN;

    check_option_from_min_to_max_and_back(sensor, RS2_OPTION_GAIN, number_of_iterations, tolerance);
    //--------------------------------------------------------------------------
    check_option_from_min_to_max_and_back(sensor, RS2_OPTION_GAIN_SECOND, number_of_iterations, tolerance);
    //--------------------------------------------------------------------------
    check_both_sensors_option_from_min_to_max_and_back(sensor, RS2_OPTION_GAIN, RS2_OPTION_GAIN_SECOND, number_of_iterations, tolerance);
    
}


void check_option_from_min_to_max_and_back(rs2::sensor sensor, rs2_option opt, int number_of_iterations, float tolerance)
{
    auto range = sensor.get_option_range(opt);
    std::cout << "-----------------------------------------------------------" << std::endl;
    std::cout << "Checking option = " << rs2_option_to_string(opt) << std::endl;

    //setting exposure to min
    float valueToSet = range.min;
    sensor.set_option(opt, valueToSet);
    float valueAfterChange = sensor.get_option(opt);
    int iterations = 5;
    while (iterations-- && valueAfterChange != valueToSet)
    {
        valueAfterChange = sensor.get_option(opt);
    }
    REQUIRE(valueAfterChange == valueToSet);
    std::cout << std::endl;
    std::cout << "Min Value Checked" << std::endl;

    //going up with step increment at each iteration until max
    checkOption(sensor, opt, valueAfterChange, range.max, number_of_iterations, tolerance, [](float value, float limit) {
        return value < limit;
        },
        [&](float& value) {
            value += range.step;
        },
            [tolerance](float valueAfterChange, float valueRequested) {
            return (valueAfterChange >= (valueRequested - tolerance)) && (valueAfterChange <= (valueRequested + tolerance));
        }
        );

    valueAfterChange = sensor.get_option(opt);
    bool result = (valueAfterChange >= (range.max - tolerance)) && (valueAfterChange <= (range.max + tolerance));
    REQUIRE(result);

    std::cout << std::endl;
    std::cout << "Option incremented and got to max value Checked" << std::endl;

    //then back to min with step decrement
    checkOption(sensor, opt, valueAfterChange, range.min, number_of_iterations, tolerance, [](float value, float limit) {
        return value > limit;
        },
        [&](float& value) {
            value -= range.step;
        },
            [tolerance](float valueAfterChange, float valueRequested) {
            return (valueAfterChange >= (valueRequested - tolerance)) && (valueAfterChange <= (valueRequested + tolerance));
        });
    valueAfterChange = sensor.get_option(opt);
    result = (valueAfterChange >= (range.min - tolerance)) && (valueAfterChange <= (range.min + tolerance));
    REQUIRE(result);

    std::cout << std::endl;
    std::cout << "Option decremented and got back to min value Checked" << std::endl;
}


void check_both_sensors_option_from_min_to_max_and_back(rs2::sensor sensor, rs2_option opt, rs2_option opt_second, int number_of_iterations, float tolerance)
{
    std::cout << "-----------------------------------------------------------" << std::endl;
    std::cout << "Checking option = " << rs2_option_to_string(opt) << " for both sensors" <<std::endl;

    auto range = sensor.get_option_range(opt);
    auto range_second = sensor.get_option_range(opt_second);
    REQUIRE(range.min == range_second.min);
    REQUIRE(range.max == range_second.max);
    REQUIRE(range.step == range_second.step);
    std::cout << "Min, Max, Step checked same for both sensors" << std::endl;

    //setting exposure to min
    float valueToSet = range.min;
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
    REQUIRE(valueAfterChange == valueToSet);
    REQUIRE(valueAfterChange_second == valueToSet);
    std::cout << std::endl;
    std::cout << "Min Value Checked" << std::endl;

    //going up with step increment at each iteration until max
    checkOptionForBothSensors(sensor, opt, opt_second, valueAfterChange, range.max, number_of_iterations, tolerance, [](float value, float limit) {
        return value < limit;
        },
        [&](float& value) {
            value += range.step;
        },
            [tolerance](float valueAfterChange, float valueRequested) {
            return (valueAfterChange >= (valueRequested - tolerance)) && (valueAfterChange <= (valueRequested + tolerance));
        }
        );

    valueAfterChange = sensor.get_option(opt);
    valueAfterChange_second = sensor.get_option(opt_second);
    bool result = (valueAfterChange >= (range.max - tolerance)) && (valueAfterChange <= (range.max + tolerance));
    REQUIRE(result);
    bool result_second = (valueAfterChange_second >= (range.max - tolerance)) && (valueAfterChange_second <= (range.max + tolerance));
    REQUIRE(result_second);

    std::cout << std::endl;
    std::cout << "Option incremented and got to max value Checked" << std::endl;

    //then back to min with step decrement
    checkOptionForBothSensors(sensor, opt, opt_second, valueAfterChange, range.min, number_of_iterations, tolerance, [](float value, float limit) {
        return value > limit;
        },
        [&](float& value) {
            value -= range.step;
        },
            [tolerance](float valueAfterChange, float valueRequested) {
            return (valueAfterChange >= (valueRequested - tolerance)) && (valueAfterChange <= (valueRequested + tolerance));
        }
        );
    valueAfterChange = sensor.get_option(opt);
    valueAfterChange_second = sensor.get_option(opt_second);
    result = (valueAfterChange >= (range.min - tolerance)) && (valueAfterChange <= (range.min + tolerance));
    REQUIRE(result);
    result_second = (valueAfterChange_second >= (range.min - tolerance)) && (valueAfterChange_second <= (range.min + tolerance));
    REQUIRE(result_second);

    std::cout << std::endl;
    std::cout << "Option decremented and got back to min value Checked" << std::endl;
}