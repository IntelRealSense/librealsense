// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "fw-update/fw-update-device.h"

namespace librealsense
{
    class ds_update_device : public update_device
    {
    public:
        ds_update_device(std::shared_ptr<context> ctx, bool register_device_notifications, std::shared_ptr<platform::usb_device> usb_device);
        virtual ~ds_update_device() = default;

        void update(const void* fw_image, int fw_image_size, update_progress_callback_ptr = nullptr) const override;

    protected:
        virtual const std::string& get_name() const override { return _name; }
        virtual const std::string& get_product_line() const override { return _product_line; }
    private:
        std::string _name;
        std::string _product_line;
    };
}
