// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "context.h"
#include "device.h"
#include <limits>

namespace librealsense
{
    /**
    * device_hub class - encapsulate the handling of connect and disconnect events
    */
    class device_hub
    {
    public:
        explicit device_hub(std::shared_ptr<librealsense::context> ctx, int mask = RS2_PRODUCT_LINE_ANY, int vid = 0, bool register_device_notifications = true);

        ~device_hub();

        /**
         * The function implements both blocking and non-blocking device generation functionality based on the input parameters:
         * Calling the function with zero timeout results in searching and fetching the device specified by the `serial` parameter
         * from a list of connected devices.
         * Calling the function with a valid timeout will block till the resulted device is found or the timeout expires.
         * \param[in] timeout_ms  The waiting period for the requested device to be reconnected (milliseconds).
         * Due to an implementation issue both in MSVC and GCC the timeout equals to 1 hour to avoid generation of
         * negative durations.
         *.\param[in] loop_through_devices  - promote internal index to the next available device that will be retrieved on successive call
         *  Note that selection of the next device is deterministic but not predictable, and therefore is recommended for
         * specific use-cases only , such as tutorials or unit-tests where no assumptions as to the device ordering are made.
         * \param[in] serial  The serial number of the requested device. Use empty pattern ("") to request for "any RealSense" device.
         * \return       a shared pointer to the device_interface that satisfies the search criteria. In case the request was not
         * resloved the call will throw an exception
         */
        std::shared_ptr<device_interface> wait_for_device(const std::chrono::milliseconds& timeout = std::chrono::milliseconds(std::chrono::hours(1)),
                                                          bool loop_through_devices = true,
                                                          const std::string& serial = "");

        /**
        * Checks if device is still connected
        */
        bool is_connected(const device_interface& dev);

        std::shared_ptr<librealsense::context> get_context();

    private:
        std::shared_ptr<device_interface> create_device(const std::string& serial, bool cycle_devices = true);
        std::shared_ptr<librealsense::context> _ctx;
        std::mutex _mutex;
        std::condition_variable _cv;
        std::vector<std::shared_ptr<device_info>> _device_list;
        int _camera_index = 0;
        int _vid = 0;
        uint64_t _device_changes_callback_id;
        bool _register_device_notifications;
    };
}
