// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 RealSense, Inc. All Rights Reserved.

#pragma once

#include "d400-device.h"

namespace librealsense
{
    class d400_device_mipi
        : public virtual d400_device,
          public virtual ds_advanced_mode_base
    {
    public:
        d400_device_mipi( std::shared_ptr< const d400_info > const & info):
            d400_device(info)
        , ds_advanced_mode_base(d400_device::_hw_monitor, get_depth_sensor()){}

        void hardware_reset() override;
        void toggle_advanced_mode(bool enable) override;

    private:
        void simulate_device_reconnect(std::shared_ptr<const device_info> dev_info);
    };
}
