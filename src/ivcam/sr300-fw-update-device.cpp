// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "sr300-fw-update-device.h"
#include "ivcam-private.h"
#include <chrono>
#include <thread>

namespace librealsense
{
    sr300_update_device::sr300_update_device(std::shared_ptr<context> ctx, bool register_device_notifications, std::shared_ptr<platform::usb_device> usb_device)
        : update_device(ctx, register_device_notifications, usb_device), _name("Intel RealSense SR300 Recovery"), _product_line("SR300")
    {

    }

    void sr300_update_device::update(const void* fw_image, int fw_image_size, update_progress_callback_ptr callback) const
    {
        update_device::update(fw_image, fw_image_size, callback);
    }
}
