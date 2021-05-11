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

#include "RsStreamLib.h"
#include "RsSafeQueue.h"

class rs_net_stream {
public:
    rs_net_stream(SoftSensor sw_sensor, rs2::stream_profile sp) : m_profile(sp), m_sw_sensor(sw_sensor) {
        m_framebuf_size = 32; // default size for motion module frames

        if (m_profile.is<rs2::video_stream_profile>()) {
            rs2::video_stream_profile vsp = m_profile.as<rs2::video_stream_profile>();
            int bpp = vsp.stream_type() == RS2_STREAM_INFRARED ? 1 : 2;
            m_framebuf_size = vsp.width() * vsp.height() * bpp;
        } 

        std::shared_ptr<uint8_t> framebuf(new uint8_t[m_framebuf_size]);
        m_framebuf = framebuf;

        std::shared_ptr<SafeQueue> net_queue(new SafeQueue);
        m_queue = net_queue;
    };

    ~rs_net_stream() {};

    void start() {
        m_doFrames_run = true;
        m_doFrames = std::thread( [&](){ doFrames(); });
    };

    void stop() {
        m_doFrames_run = false;
        if (m_doFrames.joinable()) m_doFrames.join();
    };

    rs2::stream_profile        get_profile() { return m_profile; };
    std::shared_ptr<SafeQueue> get_queue()   { return m_queue;   };

private:
    bool                        m_doFrames_run;
    std::thread                 m_doFrames;
    void doFrames();

    SoftSensor                  m_sw_sensor;

    rs2::stream_profile         m_profile;
    std::shared_ptr<SafeQueue>  m_queue;

    // Buffer to compose the frame from incoming chunks
    std::shared_ptr<uint8_t>    m_framebuf; 
    uint32_t                    m_framebuf_size;
};
