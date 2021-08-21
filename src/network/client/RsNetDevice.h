// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include <option.h>
#include <software-device.h>
#include <librealsense2/hpp/rs_internal.hpp>
#include <librealsense2/rs.hpp>

#include <list>
#include <sstream>

#include "RsNetCommon.h"
#include "RsOptions.h"
#include "RsNetSensor.h"
#include "RsNetStream.h"

#include <httplib.h>

class RsClientOptions : public RsOptions {
public:
    RsClientOptions() : RsOptions() {};
    virtual ~RsClientOptions() {};

    void init(std::vector<NetSensor> sensors) {
        m_sensors = sensors;

        for (int sensor_num = 0; sensor_num < m_sensors.size(); ++sensor_num) {
            m_sensor_options[sensor_num].init(m_sensors[sensor_num]);
        }
    };

private:
    virtual void set_opt(std::string sensor_name, RsOption option) {
        // find the sensor
        auto s = m_sensors.begin();
        for (; s != m_sensors.end(); s++) {
            std::string sname = (*s)->get_name();
            if (sname.compare(sensor_name) == 0) {
                break;
            }
        }

        try {
            LOG_DEBUG("Setting option " << (rs2_option)option.index << " #" << std::dec << option.index << " to " << option.value); // << ", old value is " << m_sensor_options[sensor_num][option.index].value);
            (*s)->set_option((rs2_option)option.index, option.value, option.range, option.ro);
        } catch(...) {
            LOG_ERROR("Failed to set option " << (rs2_option)option.index << " #" << std::dec << option.index << " for sensor " << sensor_name << " to " << option.value
                << ". Range is [" << option.range.min << ", " << option.range.max << "], default value is " << option.range.def << ", step is " << option.range.step);
        }
    };

private:
    std::vector<NetSensor> m_sensors;
};

class rs_net_device
{
public:
    rs_net_device(rs2::software_device sw_device, std::string ip_address);
   ~rs_net_device();

private:
    std::string  m_ip_address;
    unsigned int m_ip_port;

    rs2::software_device m_device;

    std::vector<NetSensor> sensors;

    void getOptions(httplib::Client& client);
    void doOptions();
    std::thread m_options;
    bool m_running;
    RsClientOptions m_opts;

    std::map<StreamPair, rs2_extrinsics> m_extrinsics_map;
    void doExtrinsics();
    std::thread m_extrinsics;
};
