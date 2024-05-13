// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "d500-fw-update-device.h"
#include "d500-private.h"


namespace librealsense
{
ds_d500_update_device::ds_d500_update_device( std::shared_ptr< const device_info > const & dev_info,
                                    std::shared_ptr< platform::usb_device > const & usb_device )
    : update_device( dev_info, usb_device, "D500" )
    {
        auto info = usb_device->get_info();
        _name = ds::rs500_sku_names.find(info.pid) != ds::rs500_sku_names.end() ? ds::rs500_sku_names.at(info.pid) : "unknown";  
        _serial_number = parse_serial_number(_serial_number_buffer);
    }


    bool ds_d500_update_device::check_fw_compatibility(const std::vector<uint8_t>& image) const
    {
        // Currently we cannot extract FW version from HKR FW image
        return true;
    }

    std::string ds_d500_update_device::parse_serial_number(const std::vector<uint8_t>& buffer) const
    {
        if (buffer.size() != sizeof(serial_number_data))
            throw std::runtime_error("DFU - failed to parse serial number!");

        std::stringstream rv;
        for (auto i = 0; i < ds::module_serial_size; i++)
            rv << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(buffer[i]);

        return rv.str();
    }

    void ds_d500_update_device::update(const void* fw_image, int fw_image_size, rs2_update_progress_callback_sptr update_progress_callback) const
    {
        update_device::update( fw_image, fw_image_size, update_progress_callback );
    }
}
