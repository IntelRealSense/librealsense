// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "device_hub.h"
#include "sync.h"

namespace librealsense
{
    class pipeline
    {
    public:
        pipeline(context& ctx);
        void start(frame_callback_ptr callback);
        void start();

        frame_holder wait_for_frames();
        ~pipeline();

     private:
        std::shared_ptr<device_interface> _dev;
        device_hub _hub;
        frame_callback_ptr _callback;
        syncer_proccess_unit _syncer;
        single_consumer_queue<frame_holder> _queue;
        std::vector<sensor_interface*> _sensors;
    };

}
