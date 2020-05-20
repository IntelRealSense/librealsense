// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.


#include "f450-common.h"


TEST_CASE( "Gain - one value - primary sensor - no streaming", "[F450]" ) {
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

    long long delay = 20;
    float tolerance = 0;
    //set AUTO EXPOSURE to manual
    sensor.set_option(RS2_OPTION_AUTO_EXPOSURE_MODE, 0);
    //std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    REQUIRE(sensor.get_option(RS2_OPTION_AUTO_EXPOSURE_MODE) == 0);

    rs2_option opt = RS2_OPTION_GAIN;
    auto range = sensor.get_option_range(opt);

    //setting exposure to one value
    float valueToSet = 65;


    sensor.set_option(RS2_OPTION_GAIN, valueToSet);
    float valueAfterChange = sensor.get_option(RS2_OPTION_GAIN);
    int iterations = 50;
    while (iterations-- && valueAfterChange != valueToSet)
    {
        //std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        valueAfterChange = sensor.get_option(RS2_OPTION_GAIN);
    }
    REQUIRE(valueAfterChange == valueToSet);
    std::cout << std::endl;
    std::cout << "Min Value Checked" << std::endl;

    
}
