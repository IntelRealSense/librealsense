// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "RsDevice.hh"
#include "RsUsageEnvironment.h"
#include <iostream>
#include <thread>

#include <librealsense2/hpp/rs_device.hpp>

int RsDevice::getPhysicalSensorUniqueKey(rs2_stream stream_type, int sensors_index)
{
    return stream_type * 10 + sensors_index;
}

RsDevice::RsDevice(UsageEnvironment* t_env, rs2::device dev)
    : env(t_env), m_device(dev)
{
    // get device sensors
    for(auto& sensor : m_device.query_sensors())
    {
        m_sensors.push_back(RsSensor(env, sensor, m_device));
    }
}

RsDevice::~RsDevice()
{
    std::cerr << "RsDevice destructor" << std::endl;
}
