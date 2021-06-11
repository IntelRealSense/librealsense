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
#include "RsNetSensor.h"
#include "RsNetStream.h"

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

    void doOptions();
    std::thread m_options;
    bool m_running;

    std::map<StreamPair, rs2_extrinsics> m_extrinsics_map;
    void doExtrinsics();
    std::thread m_extrinsics;
};
