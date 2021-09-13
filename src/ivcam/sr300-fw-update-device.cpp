// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "sr300-fw-update-device.h"
#include "sr300.h"
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

    bool sr300_update_device::check_fw_compatibility(const std::vector<uint8_t>& image) const
    {
        std::string fw_version = extract_firmware_version_string(image);
        auto min_max_fw_it = device_to_fw_min_max_version.find(_usb_device->get_info().pid);
        if (min_max_fw_it == device_to_fw_min_max_version.end())
            throw librealsense::invalid_value_exception(to_string() << "Min and Max firmware versions have not been defined for this device: " << std::hex << _pid);

        // check FW size as well, because on SR3xx it is not enough to use heuristic based on FW version
        if (image.size() != signed_sr300_size)
            throw librealsense::invalid_value_exception(to_string() << "Unsupported firmware binary image provided - " << image.size() << " bytes");

        // advanced SR3XX devices do not fit the "old" fw versions and 
        // legacy SR3XX devices do not fit the "new" fw versions
        bool result = (firmware_version(fw_version) >= firmware_version(min_max_fw_it->second.first)) &&
            (firmware_version(fw_version) <= firmware_version(min_max_fw_it->second.second));
        if (!result)
            LOG_ERROR("Firmware version isn't compatible" << fw_version);

        return result;
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
