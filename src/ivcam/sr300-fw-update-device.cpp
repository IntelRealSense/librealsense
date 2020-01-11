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
        _serial_number = parse_serial_number(_serial_number_buffer);
    }

    void sr300_update_device::update(const void* fw_image, int fw_image_size, update_progress_callback_ptr callback) const
    {
        update_device::update(fw_image, fw_image_size, callback);

        // wait for the device to come back from recovery state, TODO: check cause
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    std::string sr300_update_device::parse_serial_number(const std::vector<uint8_t>& buffer) const
    {
        if (buffer.size() != sizeof(serial_number_data))
            throw std::runtime_error("DFU - failed to parse serial number!");

        std::stringstream rv;
        for (auto i = 0; i < ivcam::module_serial_size; i++)
            rv << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(buffer[i]);

        return rv.str();
    }
}
