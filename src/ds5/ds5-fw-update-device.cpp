// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "ds5-fw-update-device.h"
#include "ds5-private.h"

namespace librealsense
{
    ds_update_device::ds_update_device(std::shared_ptr<context> ctx, bool register_device_notifications, std::shared_ptr<platform::usb_device> usb_device)
        : update_device(ctx, register_device_notifications, usb_device), _product_line("D400")
    {
        auto info = usb_device->get_info();
        _name = ds::rs400_sku_names.find(info.pid) != ds::rs400_sku_names.end() ? ds::rs400_sku_names.at(info.pid) : "unknown";        
        _serial_number = parse_serial_number(_serial_number_buffer);
    }

    void ds_update_device::update(const void* fw_image, int fw_image_size, update_progress_callback_ptr callback) const
    {
        update_device::update(fw_image, fw_image_size, callback);
    }

    std::string ds_update_device::parse_serial_number(const std::vector<uint8_t>& buffer) const
    {
        if (buffer.size() != sizeof(serial_number_data))
            throw std::runtime_error("DFU - failed to parse serial number!");

        std::stringstream rv;
        for (auto i = 0; i < ds::module_serial_size; i++)
            rv << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(buffer[i]);

        return rv.str();
    }
}
