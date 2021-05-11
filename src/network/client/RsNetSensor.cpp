// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <httplib.h>

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include "RsNetDevice.h"

#include <api.h>
#include <librealsense2-net/rs_net.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <thread>
#include <functional>

#include <zstd.h>
#include <zstd_errors.h>

#include <lz4.h>
#include <jpeg.h>

#include <stdlib.h>
#include <math.h>

using namespace std::placeholders;

void rs_net_sensor::doRTP() {
    std::stringstream ss;
    ss << std::setiosflags(std::ios::left) << std::setw(14) << m_name << ": RTP support thread started";
    LOG_INFO(ss.str());

    TaskScheduler* scheduler = BasicTaskScheduler::createNew(/* 1000 */); // Check this later
    m_env = BasicUsageEnvironment::createNew(*scheduler);

    // Start the watch thread  
    m_env->taskScheduler().scheduleDelayedTask(100000, doControl, this);

    // Start the scheduler
    m_env->taskScheduler().doEventLoop(&m_eventLoopWatchVariable);

    LOG_INFO(m_name << " : RTP support thread exited");
}

void rs_net_sensor::doControl() {
    bool streaming = is_streaming();
    if (streaming != m_streaming) {
        // sensor state changed
        m_streaming = streaming;
        if (is_streaming()) {
            LOG_INFO("Sensor enabled");

            // Create RTSP client
            RTSPClient::responseBufferSize = 100000;
            m_rtspClient = RSRTSPClient::createNew(*m_env, m_mrl.c_str());
            if (m_rtspClient == NULL) {
                LOG_FATAL("Failed to create a RTSP client for URL '" << m_mrl << "': " << m_env->getResultMsg());
                throw std::runtime_error("Cannot create RTSP client");
            }
            LOG_INFO("Connected to " << m_mrl);

            // Prepare profiles list and allocate the queues
            m_streams.clear();
            for (auto stream_profile : m_sw_sensor->get_active_streams()) {
                rs_net_stream* net_stream = new rs_net_stream(m_sw_sensor, stream_profile);
                uint64_t key = slib::profile2key(net_stream->get_profile());
                m_streams[key] = net_stream;
            }
            
            // Start playing streams
            m_rtspClient->playStreams(m_streams);

            // Start SW device thread
            for (auto ks : m_streams) {
                rs_net_stream* net_stream = ks.second;
                net_stream->start();
            }
        } else {
            LOG_INFO("Sensor disabled");

            // disable running RTP sessions
            m_rtspClient->shutdownStream();
            m_rtspClient = NULL;

            // Stop SW device thread
            for (auto ks : m_streams) {
                auto net_stream = ks.second;
                net_stream->stop();
                delete net_stream;
            }
            m_streams.clear();
        }
    }

    m_env->taskScheduler().scheduleDelayedTask(100000, doControl, this);
}
