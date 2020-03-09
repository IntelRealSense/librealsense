// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <iostream>
#include "RsDevice.hh"

RsDevice::RsDevice()
{
        //get LRS device
        // The context represents the current platform with respect to connected devices
        rs2::context ctx;
        rs2::device_list devices = ctx.query_devices();
        rs2::device dev;
        if (devices.size() == 0)
        {
                std::cerr << "No device connected, please connect a RealSense device" << std::endl;
                rs2::device_hub device_hub(ctx);
                m_device = device_hub.wait_for_device(); //todo: check wait_for_device
        }
        else
        {
                m_device = devices[0]; // Only one device is supported
        }

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
