// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "fw-update/fw-update-device.h"

namespace librealsense
{
    class ds_d500_update_device : public update_device
    {
    public:
        ds_d500_update_device( std::shared_ptr< const device_info > const &,
                          std::shared_ptr< platform::usb_device > const & usb_device );
        virtual ~ds_d500_update_device() = default;

        virtual bool check_fw_compatibility(const std::vector<uint8_t>& image) const override;
        virtual void update(const void* fw_image, int fw_image_size, rs2_update_progress_callback_sptr = nullptr) const override;
        virtual bool wait_for_manifest_completion(std::shared_ptr<platform::usb_messenger> messenger, const rs2_dfu_state state,
            std::chrono::seconds timeout_seconds, rs2_update_progress_callback_sptr update_progress_callback) const override;

    private:
        std::string parse_serial_number(const std::vector<uint8_t>& buffer) const;
    };
}
