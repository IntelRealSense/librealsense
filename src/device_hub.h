// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "context.h"
#include "device.h"

namespace librealsense
{
    /**
    * device_hub class - encapsulate the handling of connect and disconnect events
    */
    class device_hub
    {
    public:
        explicit device_hub(context& ctx);

        /**
         * If any device is connected return it, otherwise wait until next RealSense device connects.
         * Calling this method multiple times will cycle through connected devices
         */
        std::shared_ptr<device_interface> wait_for_device();

        /**
        * Checks if device is still connected
        */
        bool is_connected(const device& dev);

    private:
        context& _ctx;
        std::mutex _mutex;
        std::condition_variable _cv;
        std::vector<std::shared_ptr<device_info>> _device_list;
        int _camera_index = 0;
    };
}


