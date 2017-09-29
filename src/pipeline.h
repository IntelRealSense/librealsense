// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "device_hub.h"
#include "sync.h"
#include "config.h"
#include <map>
#include <utility>

namespace librealsense
{
    class pipeline;
    class pipeline_profile
    {
    public:
        pipeline_profile(std::shared_ptr<device_interface> dev, util::config::multistream multistream, const std::string& file);
        std::shared_ptr<device_interface> get_device();
        stream_profiles get_active_streams() const;
        util::config::multistream _multistream;
    private:
        std::shared_ptr<device_interface> _dev;
        std::string _to_file;
    };

    class pipeline_config;
    class pipeline : public std::enable_shared_from_this<pipeline>
    {
    public:
        //Top level API
        explicit pipeline(std::shared_ptr<librealsense::context> ctx);
        ~pipeline();
        std::shared_ptr<pipeline_profile> start(std::shared_ptr<pipeline_config> conf);
        std::shared_ptr<pipeline_profile> start_with_record(std::shared_ptr<pipeline_config> conf, const std::string& file);
        void stop();
        std::shared_ptr<pipeline_profile> get_active_profile() const;
        frame_holder wait_for_frames(unsigned int timeout_ms = 5000);
        bool poll_for_frames(frame_holder* frame);

        //Non top level API
        std::shared_ptr<device_interface> wait_for_device(unsigned int timeout_ms = std::numeric_limits<unsigned int>::max(), std::string serial = "");
        std::shared_ptr<librealsense::context> get_context() const;


     private:
         void unsafe_start(std::shared_ptr<pipeline_config> conf);
         void unsafe_stop();
        std::shared_ptr<librealsense::context> _ctx;
        std::mutex _mtx;
        device_hub _hub;
        std::shared_ptr<pipeline_profile> _active_profile;
        frame_callback_ptr _callback;
        std::unique_ptr<syncer_proccess_unit> _syncer;
        std::unique_ptr<single_consumer_queue<frame_holder>> _queue;

        std::shared_ptr<pipeline_config> _prev_conf;
    };


    class pipeline_config
    {
    public:
        void enable_stream(rs2_stream stream, int index, int width, int height, rs2_format format, int framerate);
        void enable_all_stream();
        void enable_device(const std::string& serial);
        void enable_device_from_file(const std::string& file);
        void disable_stream(rs2_stream stream);
        void disable_all_streams();
        std::shared_ptr<pipeline_profile> resolve(std::shared_ptr<pipeline> pipe);
        bool can_resolve(std::shared_ptr<pipeline> pipe);

        //Non top level API
        void enable_record_to_file(const std::string& file);

    private:
        struct device_request
        {
            std::string serial;
            std::string filename;
            std::string record_output;
        };

        std::shared_ptr<device_interface> resolve_device_requests(std::shared_ptr<pipeline> pipe);
        stream_profiles get_default_configuration(std::shared_ptr<device_interface> dev);
        std::shared_ptr<device_interface> get_first_or_default_device(std::shared_ptr<pipeline> pipe);
        device_request _device_request;
        std::map<std::pair<rs2_stream, int>, util::config::request_type> _stream_requests;
        std::mutex _mtx;
        bool _enable_all_streams = false;
    };

}
