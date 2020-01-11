// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "core/streaming.h"

namespace librealsense
{
    class updatable
    {
    public:
        virtual void enter_update_state() const = 0;
        virtual std::vector<uint8_t> backup_flash(update_progress_callback_ptr callback) = 0;
        virtual void update_flash(const std::vector<uint8_t>& image, update_progress_callback_ptr callback, int update_mode) = 0;
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
