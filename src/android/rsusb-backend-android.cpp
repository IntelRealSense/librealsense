// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "rsusb-backend-android.h"
#include "device_watcher.h"

namespace librealsense
{
    namespace platform
    {
        std::shared_ptr<backend> create_backend()
        {
            return std::make_shared<android_backend>();
        }

        std::shared_ptr<device_watcher> android_backend::create_device_watcher() const
        {
            return device_watcher_usbhost::instance();
        }
    }
}
