// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <liveMedia.hh>
#include <GroupsockHelper.hh>
#include <BasicUsageEnvironment.hh>
#include <RTSPCommon.hh>

#include <librealsense2/hpp/rs_internal.hpp>
#include <librealsense2/rs.hpp>

#include <string>

#include "RsStreamLib.h"
#include "RsNetCommon.h"
#include "RsOptions.h"

class RsServerOptions : public RsOptions {
public:
    RsServerOptions() : RsOptions() {};
    virtual ~RsServerOptions() {};

    void init(std::vector<rs2::sensor> sensors) {
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
            std::string sname(s->supports(RS2_CAMERA_INFO_NAME) ? s->get_info(RS2_CAMERA_INFO_NAME) : "Unknown");
            if (sname.compare(sensor_name) == 0) {
                break;
            }
        }

        try {
            if (!s->is_option_read_only((rs2_option)option.index)) {
                LOG_INFO("Setting option " << (rs2_option)option.index << " #" << std::dec << option.index << " to " << option.value);
                s->set_option((rs2_option)option.index, option.value);
            }
        } catch(...) {
            LOG_ERROR("Failed to set option " << (rs2_option)option.index << " #" << std::dec << option.index << " for sensor " << sensor_name << " to " << option.value
                << ". Range is [" << option.range.min << ", " << option.range.max << "], default value is " << option.range.def << ", step is " << option.range.step);
        }
    };

private:
    std::vector<rs2::sensor> m_sensors;
};

class server
{
public:
    server(rs2::device dev, std::string addr, int port);
    ~server();

    void start();
    void stop();

private:
    UsageEnvironment* env;
    TaskScheduler* scheduler;

    rs2::device m_dev;

    RTSPServer* srv;

    std::thread m_httpd;
    void doHTTP();

    std::thread m_fw_upgrade;
    void doFW_Upgrade();
    std::string m_image;
    
    std::string m_progress;

    std::string m_sensors_desc; // sensors description
    std::stringstream m_extrinsics;   // streams extrinsics

    RsServerOptions m_opts;
};
