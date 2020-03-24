// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "software-device.h"
#include <librealsense2/rs.hpp>

#include <list>

// Holds ip sensor data.
//     1. sw sensor
//     2. relative streams
//     3. state -> on/off

class ip_sensor
{
public:
    //todo: remove smart ptr here
    std::shared_ptr<rs2::software_sensor> sw_sensor;

    std::list<long long int> active_streams_keys;

    std::map<rs2_option, float> sensors_option;

    bool is_enabled;

    //TODO: get smart ptr from rtsp client creator
    IRsRtsp* rtsp_client;

    ip_sensor(/* args */);
    ~ip_sensor();
};

ip_sensor::ip_sensor(/* args */)
{
    is_enabled = false;
}

ip_sensor::~ip_sensor() {}
