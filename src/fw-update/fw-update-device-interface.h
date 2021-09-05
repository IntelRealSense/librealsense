// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "../types.h"
#include "../core/streaming.h"
#include <cstddef>

namespace librealsense
{
    class firmware_check_interface
    {
    public:
        virtual bool check_fw_compatibility(const std::vector<uint8_t>& image) const = 0;
        static std::string extract_firmware_version_string(const std::vector<uint8_t>& fw_image)
        {
            auto version_offset = offsetof(platform::dfu_header, bcdDevice);
            if (fw_image.size() < (version_offset + sizeof(size_t)))
                throw std::runtime_error("Firmware binary image might be corrupted - size is only: " + fw_image.size());

            auto version = fw_image.data() + version_offset;
            uint8_t major = *(version + 3);
            uint8_t minor = *(version + 2);
            uint8_t patch = *(version + 1);
            uint8_t build = *(version);

            return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch) + "." + std::to_string(build);
        }
    };

    class updatable : public firmware_check_interface
    {
    public:
        virtual void enter_update_state() const = 0;
        virtual std::vector<uint8_t> backup_flash(update_progress_callback_ptr callback) = 0;
        virtual void update_flash(const std::vector<uint8_t>& image, update_progress_callback_ptr callback, int update_mode) = 0;
    };

    class update_device_interface : public device_interface, public firmware_check_interface 
    {
    public:
        virtual void update(const void* fw_image, int fw_image_size, update_progress_callback_ptr = nullptr) const = 0;
    protected:
        virtual const std::string& get_name() const = 0;
        virtual const std::string& get_product_line() const = 0;
        virtual const std::string& get_serial_number() const = 0;
    };

    MAP_EXTENSION(RS2_EXTENSION_UPDATE_DEVICE, update_device_interface);
    MAP_EXTENSION(RS2_EXTENSION_UPDATABLE, updatable);
}
