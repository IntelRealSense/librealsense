// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <src/platform/platform-device-info.h>


namespace librealsense
{
    class d500_info : public platform::platform_device_info
    {
    public:
        std::shared_ptr< device_interface > create_device() override;

        d500_info( std::shared_ptr< context > const & ctx,
                   std::vector< platform::uvc_device_info > && depth,
                   std::vector< platform::usb_device_info > && hwm,
                   std::vector< platform::hid_device_info > && hid )
            : platform_device_info( ctx, { std::move( depth ), std::move( hwm ), std::move( hid ) } )
        {
        }

        static std::vector<std::shared_ptr<d500_info>> pick_d500_devices(
                std::shared_ptr<context> ctx,
                platform::backend_device_group& group);
    };
}
