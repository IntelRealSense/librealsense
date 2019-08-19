// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once


#include "core/streaming.h"
#include "context.h"

namespace librealsense
{
    class fw_update_info : public device_info
    {
    public:
        std::shared_ptr<device_interface> create(std::shared_ptr<context> ctx, bool register_device_notifications) const override;

        static std::vector<std::shared_ptr<device_info>> pick_recovery_devices(std::shared_ptr<context> ctx,
            const std::vector<platform::usb_device_info>& usb_devices, int mask);

        explicit fw_update_info(std::shared_ptr<context> ctx, platform::usb_device_info dfu)
            : device_info(ctx), _dfu(std::move(dfu)) {}

        platform::backend_device_group get_device_data()const override { return platform::backend_device_group({ _dfu }); }

    private:
        platform::usb_device_info _dfu;
        const char* RECOVERY_MESSAGE = "Selected RealSense device is in recovery mode!\nEither perform a firmware update or reconnect the camera to fall-back to last working firmware if available!";
    };
}
