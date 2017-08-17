// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "device_hub.h"
#include "sync.h"
#include "config.h"

namespace librealsense
{
    class pipeline
    {
    public:
        pipeline(context& ctx);
        void start(frame_callback_ptr callback);
        void start();
        void enable(rs2_stream stream, int index, uint32_t width, uint32_t height, rs2_format format, uint32_t framerate);
        void disable_stream(rs2_stream stream);
        void disable_all();

        frame_holder wait_for_frames(unsigned int timeout_ms = 5000);
        ~pipeline();

     private:
        std::shared_ptr<device_interface> _dev;
        device_hub _hub;
        frame_callback_ptr _callback;
        syncer_proccess_unit _syncer;
        single_consumer_queue<frame_holder> _queue;
        std::vector<sensor_interface*> _sensors;
        util::config _config;
        util::config::multistream _multistream;
        std::vector<stream_profile> _profiles;
    };

}
