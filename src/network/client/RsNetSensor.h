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

#include "RsNetStream.h"
#include "RsStreamLib.h"
#include "RsRTSPClient.h"

class rs_net_sensor {
public: 
    rs_net_sensor(SoftSensor sw_sensor, std::string name)
        : m_sw_sensor(sw_sensor), m_name(name), m_streaming(false), m_dev_flag(false), m_eventLoopWatchVariable(0) {};

    ~rs_net_sensor() {};

    std::string get_name() { return m_name; }
    SoftSensor get_sensor() { return m_sw_sensor; }

    void set_mrl(std::string mrl) { m_mrl  = mrl; };

    void add_profile(uint64_t key, rs2_intrinsics intrinsics) { 
        rs2_video_stream  vstream = slib::key2stream(key);
        rs2_motion_stream mstream;

        std::stringstream ss_profile;
        ss_profile << "Profile : " << slib::print_stream(&vstream);

        switch (vstream.type) {
        case RS2_STREAM_DEPTH      :
        case RS2_STREAM_COLOR      :
        case RS2_STREAM_INFRARED   :
        case RS2_STREAM_CONFIDENCE :
            vstream.intrinsics = intrinsics;
            m_sw_sensor->add_video_stream(vstream, slib::is_default(key));

            // in case of the color stream add four additional formats
            // we are going to convert them on host from YUYV
            if (vstream.type == RS2_STREAM_COLOR) {
                vstream.fmt = RS2_FORMAT_BGRA8;
                m_sw_sensor->add_video_stream(vstream, slib::is_default(key));

                vstream.fmt = RS2_FORMAT_RGBA8;
                m_sw_sensor->add_video_stream(vstream, slib::is_default(key));

                vstream.fmt = RS2_FORMAT_BGR8;
                m_sw_sensor->add_video_stream(vstream, slib::is_default(key));

                vstream.fmt = RS2_FORMAT_RGB8;
                m_sw_sensor->add_video_stream(vstream, slib::is_default(key));
            }
            break;
        case RS2_STREAM_GYRO     :
        case RS2_STREAM_ACCEL    :
            mstream.type  = vstream.type;
            mstream.index = vstream.index;
            mstream.uid   = vstream.uid; // software-device.cpp:124
            mstream.fps   = vstream.fps;
            mstream.fmt   = vstream.fmt;
            m_sw_sensor->add_motion_stream(mstream, slib::is_default(key));
            break;
        default:
            throw std::runtime_error("Unsupported stream type");
        }

        LOG_INFO(ss_profile.str());
    };

    void add_option(uint32_t idx, float val, rs2::option_range range) {
        rs2_option opt = static_cast<rs2_option>(idx);

        m_sw_sensor->add_option(opt, range);

        try {
            m_sw_sensor->set_option(opt, val);
        } catch (const rs2::error& e) {
            // Some options can only be set while the camera is streaming,
            // and generally the hardware might fail so it is good practice to catch exceptions from set_option
            LOG_ERROR("Failed to set option " << opt << ". (" << e.what() << ")");
        }
    }

    bool is_streaming() { return m_sw_sensor->get_active_streams().size() > 0; };

    void start() {
        m_rtp = std::thread( [&](){ doRTP(); });
    };

    void doRTP();
    static void doControl(void* clientData) {
        rs_net_sensor* sensor = (rs_net_sensor*)clientData;
        sensor->doControl(); 
    };
    void doControl();

private:    
    rs2::software_device m_sw_device;

    std::string    m_name;
    std::string    m_mrl;
    std::string    m_opts;

    SoftSensor     m_sw_sensor;

    std::map<uint64_t, rs_net_stream*> m_streams;

    std::thread    m_rtp;
    volatile bool  m_dev_flag;

    RSRTSPClient*  m_rtspClient;
    char m_eventLoopWatchVariable;

    bool m_streaming;

    UsageEnvironment* m_env;
};

using NetSensor = std::shared_ptr<rs_net_sensor>;
