// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "rsusb-backend-windows.h"
#include "types.h"
#include "../uvc/uvc-device.h"

namespace librealsense
{
    namespace platform
    {
        std::shared_ptr<backend> create_backend()
        {
            return std::make_shared<rs_backend_windows>();
        }

        std::shared_ptr<device_watcher> rs_backend_windows::create_device_watcher() const
        {
            return std::make_shared<polling_device_watcher>(this);
        }
    }
}
