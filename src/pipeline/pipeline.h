// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <utility>

#include "device_hub.h"
#include "sync.h"
#include "profile.h"
#include "config.h"
#include "resolver.h"
#include "aggregator.h"

namespace librealsense
{
    namespace pipeline
    {
        class pipeline : public std::enable_shared_from_this<pipeline>
        {
        public:
            //Top level API
            explicit pipeline(std::shared_ptr<librealsense::context> ctx);
            virtual ~pipeline();
            std::shared_ptr<profile> start(std::shared_ptr<config> conf, frame_callback_ptr callback = nullptr);
            void stop();
            std::shared_ptr<profile> get_active_profile() const;
            frame_holder wait_for_frames(unsigned int timeout_ms);
            bool poll_for_frames(frame_holder* frame);
            bool try_wait_for_frames(frame_holder* frame, unsigned int timeout_ms);

            //Non top level API
            std::shared_ptr<device_interface> wait_for_device(const std::chrono::milliseconds& timeout = std::chrono::hours::max(),
                const std::string& serial = "");
            std::shared_ptr<librealsense::context> get_context() const;

        protected:
            frame_callback_ptr get_callback(std::vector<int> unique_ids);
            std::vector<int> on_start(std::shared_ptr<profile> profile);

            void unsafe_start(std::shared_ptr<config> conf);
            void unsafe_stop();

            mutable std::mutex _mtx;
            std::shared_ptr<profile> _active_profile;
            device_hub _hub;
            std::shared_ptr<config> _prev_conf;

        private:
            std::shared_ptr<profile> unsafe_get_active_profile() const;

            std::shared_ptr<librealsense::context> _ctx;
            int _playback_stopped_token = -1;
            dispatcher _dispatcher;

            std::unique_ptr<syncer_process_unit> _syncer;
            std::unique_ptr<aggregator> _aggregator;

            frame_callback_ptr _streams_callback;
            std::vector<rs2_stream> _synced_streams;
        };
    }
}
