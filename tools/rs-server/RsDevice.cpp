// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <thread>
#include "RsDevice.hh"

int RsDevice::getPhysicalSensorUniqueKey(rs2_stream stream_type, int sensors_index)
{ 
  return stream_type * 10 + sensors_index;
}

RsDevice::RsDevice()
{
    //get LRS device
    // The context represents the current platform with respect to connected devices

    bool found = false;
    bool first = true;
    do 
    {
        rs2::context ctx;
        rs2::device_list devices = ctx.query_devices();
        rs2::device dev;
        if (devices.size())
        {
            try
            {
                m_device = devices[0]; // Only one device is supported
                std::cerr << "RealSense Device Connected" << std::endl;
                
                found = true;
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
        }
        if (!found) 
        {
            if (first) 
            {
                std::cerr << "Waiting for Device..." << std::endl;
                first = false;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    while (!found);

    //get RS sensors
    for (auto &sensor : m_device.query_sensors())
    {
        m_sensors.push_back(RsSensor(sensor, m_device));
    }
}

RsDevice::~RsDevice()
{
        std::cerr << "RsDevice destructor" << std::endl;
}
