// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "fw-update/fw-update-device.h"

namespace librealsense
{
    class ds_d400_update_device : public update_device
    {
    public:
        ds_d400_update_device( std::shared_ptr< const device_info > const &,
                               std::shared_ptr< platform::usb_device > const & usb_device );
        virtual ~ds_d400_update_device() = default;

        virtual bool check_fw_compatibility(const std::vector<uint8_t>& image) const override;

    private:
        std::string parse_serial_number(const std::vector<uint8_t>& buffer) const;
    };
}
