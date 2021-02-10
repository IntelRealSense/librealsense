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

#include "RsSafeQueue.h"

class rs_net_stream {
public:
    rs_net_stream(rs2::stream_profile sp) : profile(sp) {
        if (profile.is<rs2::video_stream_profile>()) {
            rs2::video_stream_profile vsp = sp.as<rs2::video_stream_profile>();
            int bpp = vsp.stream_type() == RS2_STREAM_INFRARED ? 1 : 2;
            m_frame_raw = new uint8_t[vsp.width() * vsp.height() * bpp];
        } else {
            m_frame_raw = new uint8_t[32];
        }

        queue   = new SafeQueue;
    };
    ~rs_net_stream() {
        if (m_frame_raw) delete [] m_frame_raw;
        if (queue) delete queue;
    };

    rs2_video_stream     stream;
    rs2::stream_profile  profile;
    SafeQueue*           queue;
    std::thread          thread;

    uint8_t*             m_frame_raw;
};
