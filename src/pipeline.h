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
        pipeline(std::shared_ptr<librealsense::context> ctx);
        std::shared_ptr<device_interface> get_device();
        void start(frame_callback_ptr callback);
        void start();
        void stop();
        void open();
        void enable(std::string device_serial);
        void enable(rs2_stream stream, int index, uint32_t width, uint32_t height, rs2_format format, uint32_t framerate);
        void disable_stream(rs2_stream stream);
        void disable_all();

        frame_holder wait_for_frames(unsigned int timeout_ms = 5000);
        bool poll_for_frames(frame_holder* frame);

        stream_profiles get_active_streams() const;

        ~pipeline();

     private:
        std::shared_ptr<librealsense::context> _ctx;
        std::recursive_mutex _mtx;
        std::shared_ptr<device_interface> _dev;
        device_hub _hub;
        frame_callback_ptr _callback;
        syncer_proccess_unit _syncer;
        single_consumer_queue<frame_holder> _queue;
        std::vector<sensor_interface*> _sensors;
        util::config _config;
        util::config::multistream _multistream;
        bool _commited = false;
        bool _streaming = false; 
    };

}
