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

    bool l500_update_device::check_fw_compatibility(const std::vector<uint8_t>& image) const
    {
        std::string fw_version = extract_firmware_version_string(image);
        auto min_max_fw_it = ivcam2::device_to_fw_min_max_version.find(_usb_device->get_info().pid);
        if (min_max_fw_it == ivcam2::device_to_fw_min_max_version.end())
            throw librealsense::invalid_value_exception(to_string() << "Min and Max firmware versions have not been defined for this device: " << std::hex << _pid);

        // Limit L515 to FW versions within the 1.5.1.3-1.99.99.99 range to differenciate from the other products
        bool result = (firmware_version(fw_version) >= firmware_version(min_max_fw_it->second.first)) &&
            (firmware_version(fw_version) <= firmware_version(min_max_fw_it->second.second));
        if (!result)
            LOG_ERROR("Firmware version isn't compatible" << fw_version);

        return result;
    }

    std::string l500_update_device::parse_serial_number(const std::vector<uint8_t>& buffer) const
    {
        // Note that we are using a specific serial_number_data struct then the generic one.
        // See comment in the struct definition for more details
        if (buffer.size() != sizeof(l500_update_device::serial_number_data))
            throw std::runtime_error("DFU - failed to parse serial number!");

        const auto serial_num_data = (l500_update_device::serial_number_data *)buffer.data();
        std::stringstream rv;
        for (auto i = 0; i < ivcam2::module_asic_serial_size; i++)
            rv << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(serial_num_data->serial[i]);

        return rv.str();
    }

}
