// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "l500-fw-update-device.h"
#include "l500-private.h"

namespace librealsense
{
    l500_update_device::l500_update_device(std::shared_ptr<context> ctx, bool register_device_notifications, std::shared_ptr<platform::usb_device> usb_device)
        : update_device(ctx, register_device_notifications, usb_device), _product_line("L500")
    {
        auto info = usb_device->get_info();
        _name = ivcam2::rs500_sku_names.find(info.pid) != ivcam2::rs500_sku_names.end() ? ivcam2::rs500_sku_names.at(info.pid) : "unknown";
        _serial_number = parse_serial_number(_serial_number_buffer);
    }

    void l500_update_device::update(const void* fw_image, int fw_image_size, update_progress_callback_ptr callback) const
    {
        update_device::update(fw_image, fw_image_size, callback);
    }

    std::string l500_update_device::parse_serial_number(const std::vector<uint8_t>& buffer) const
    {
        if (buffer.size() != sizeof(serial_number_data))
            throw std::runtime_error("DFU - failed to parse serial number!");

        auto rev = buffer;

        std::stringstream rv;
        for (auto i = 0; i < ivcam2::module_serial_size; i++)
            rv << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(rev[i]);

        return rv.str();
    }
}
