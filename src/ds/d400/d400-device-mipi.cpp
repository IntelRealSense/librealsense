// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 RealSense, Inc. All Rights Reserved.

#include "d400-device-mipi.h"

namespace librealsense
{

    void d400_device_mipi::simulate_device_reconnect(std::shared_ptr<const device_info> dev_info)
    {
        //limitation: the user must hold the context from which the device was created
        //creating fake notification to trigger invoke_devices_changed_callbacks, causing disconnection and connection
        auto non_const_device_info = std::const_pointer_cast<librealsense::device_info>(dev_info);
        std::vector< std::shared_ptr< device_info > > devices{ non_const_device_info };
        auto ctx = std::weak_ptr< context >(dev_info->get_context());
        std::thread fake_notification(
            [ctx, devs = std::move(devices)]()
            {
                try
                {
                    if (auto strong = ctx.lock())
                    {
                        strong->invoke_devices_changed_callbacks(devs, {});
                        // MIPI devices do not re-enumerate so we need to give them some time to restart
                        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
                    }
                    if (auto strong = ctx.lock())
                        strong->invoke_devices_changed_callbacks({}, devs);
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR(e.what());
                    return;
                }
            });
        fake_notification.detach();
    }

    void d400_device_mipi::hardware_reset() override
    {
        d400_device::hardware_reset();
        simulate_device_reconnect(this->get_device_info());
    }

    void d400_device_mipi::toggle_advanced_mode(bool enable) override
    {
        ds_advanced_mode_base::toggle_advanced_mode(enable);
        simulate_device_reconnect(this->get_device_info());
    }
}
