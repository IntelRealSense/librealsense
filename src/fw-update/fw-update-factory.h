// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "platform/platform-device-info.h"

namespace librealsense
{
    class context;

    class fw_update_info : public platform::platform_device_info
    {
        typedef platform::platform_device_info super;

    public:
        std::shared_ptr< device_interface > create_device() override;

        static std::vector< std::shared_ptr< fw_update_info > > pick_recovery_devices(
            std::shared_ptr< context > ctx, const std::vector< platform::usb_device_info > & usb_devices, int mask );

        explicit fw_update_info(std::shared_ptr<context> ctx, platform::usb_device_info const & dfu)
            : platform_device_info( ctx, { { dfu } } ) {}

        std::string get_address() const override { return "recovery:" + super::get_address(); }

    private:
        const char* RECOVERY_MESSAGE = "Selected RealSense device is in recovery mode!\nEither perform a firmware update or reconnect the camera to fall-back to last working firmware if available!";
    };
}
