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
        
        void start();
        void stop();

        void enable(std::string device_serial);
        void enable(rs2_stream stream, int index, uint32_t width, uint32_t height, rs2_format format, uint32_t framerate);
        void commit_config();
        void reset_config();

        frame_holder wait_for_frames(unsigned int timeout_ms = 5000);
        bool poll_for_frames(frame_holder* frame);

        std::shared_ptr<device_interface> get_device();
        stream_profiles get_active_streams() const;

        ~pipeline();

     private:

        class resolver
        {
        public:
            resolver(std::shared_ptr<librealsense::context> ctx);
            std::pair<std::shared_ptr<device_interface>, util::config::multistream> resolve();
            void enable(rs2_stream stream, int index, uint32_t width, uint32_t height, rs2_format format, uint32_t framerate);
            void enable(std::string device_serial);

        private:
            util::config _config;
            std::string _device_serial;
            device_hub _hub;
            std::shared_ptr<librealsense::context> _ctx;
        };

        std::shared_ptr<librealsense::context> _ctx;
        std::recursive_mutex _mtx;
        std::shared_ptr<device_interface> _dev;
        
        syncer_proccess_unit _syncer;
        single_consumer_queue<frame_holder> _queue;

      

        bool _commited = false;
        bool _streaming = false; 
    };

}
