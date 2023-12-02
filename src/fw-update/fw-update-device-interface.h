// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "../types.h"
#include <src/core/device-interface.h>
#include "../usb/usb-types.h"
#include <cstddef>

namespace librealsense
{
    class firmware_check_interface
    {
    public:
        virtual bool check_fw_compatibility(const std::vector<uint8_t>& image) const = 0;
    };

    class updatable : public firmware_check_interface
    {
    public:
        virtual void enter_update_state() const = 0;
        virtual std::vector<uint8_t> backup_flash( rs2_update_progress_callback_sptr callback) = 0;
        virtual void update_flash(const std::vector<uint8_t>& image, rs2_update_progress_callback_sptr callback, int update_mode) = 0;
    };

    MAP_EXTENSION( RS2_EXTENSION_UPDATABLE, updatable );

    class update_device_interface : public device_interface, public firmware_check_interface 
    {
    public:
        virtual void update(const void* fw_image, int fw_image_size, rs2_update_progress_callback_sptr = nullptr) const = 0;
    protected:
        virtual const std::string& get_name() const = 0;
        virtual const std::string& get_product_line() const = 0;
        virtual const std::string& get_serial_number() const = 0;
    };

    MAP_EXTENSION(RS2_EXTENSION_UPDATE_DEVICE, update_device_interface);
}
