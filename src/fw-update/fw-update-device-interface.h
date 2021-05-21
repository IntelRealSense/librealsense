// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "../types.h"
#include "../core/streaming.h"
#include <cstddef>

namespace librealsense
{
    class updatable
    {
    public:
        virtual void enter_update_state() const = 0;
        virtual std::vector<uint8_t> backup_flash(update_progress_callback_ptr callback) = 0;
        virtual void update_flash(const std::vector<uint8_t>& image, update_progress_callback_ptr callback, int update_mode) = 0;
        virtual bool check_fw_compatibility(const std::vector<uint8_t>& image) const = 0;
        std::string extract_firmware_version_string(const void* fw_image, size_t fw_image_size) const
        {
            if (!fw_image)
                throw std::runtime_error("Firmware binary image might be corrupted - null pointer");
            
            auto version_offset = offsetof(platform::dfu_header, bcdDevice);

            if (fw_image_size < (version_offset + sizeof(size_t)))
                throw std::runtime_error("Firmware binary image might be corrupted - size is only: " + fw_image_size);

            uint32_t version{};

            memcpy(reinterpret_cast<char*>(&version), reinterpret_cast<const char*>(fw_image) +
                version_offset, sizeof(version));

            uint8_t major = (version & 0xFF000000) >> 24;
            uint8_t minor = (version & 0x00FF0000) >> 16;
            uint8_t patch = (version & 0x0000FF00) >> 8;
            uint8_t build = version & 0x000000FF;

            return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch) + "." + std::to_string(build);
        }
    };

    class update_device_interface : public device_interface
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
