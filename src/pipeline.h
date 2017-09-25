// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "device_hub.h"
#include "sync.h"
#include "config.h"

namespace librealsense
{
    struct configuration
    {
        configuration(std::shared_ptr<device_interface> dev,
            util::config::multistream multistream,
            std::shared_ptr<device_hub> hub)
            :_dev(dev), _multistream(multistream), _hub(hub) {}

        bool device_disconnected();
        std::shared_ptr<device_interface> _dev;
        util::config::multistream _multistream;
        std::shared_ptr<device_hub> _hub;
    };

    class resolver
    {
    public:
        resolver(std::shared_ptr<librealsense::context> ctx);
        std::shared_ptr<configuration> resolve(unsigned int timeout_ms = std::numeric_limits<uint64_t>::max());
        void enable(rs2_stream stream, int index, uint32_t width, uint32_t height, rs2_format format, uint32_t framerate);
        void enable(std::string device_serial);

    private:

        void set_default_configuration(device_interface* dev);

        util::config _config;
        bool _default_configuration = false;
        std::string _device_serial;
        std::shared_ptr<device_hub> _hub;
        std::shared_ptr<librealsense::context> _ctx;
    };

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
         void reconfig(unsigned int timeout_ms = std::numeric_limits<uint64_t>::max());

         mutable std::recursive_mutex _mtx;

        std::shared_ptr<resolver> _resolver;
        std::shared_ptr<configuration> _configuration;

        syncer_proccess_unit _syncer;
        single_consumer_queue<frame_holder> _queue;

        std::shared_ptr<librealsense::context> _ctx;

        bool _commited = false;
        bool _streaming = false; 
    };

}
