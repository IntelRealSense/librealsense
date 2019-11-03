// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "../rsusb-backend/rsusb-backend.h"

namespace librealsense
{
    namespace platform
    {
        class android_backend : public rs_backend
        {
        public:
            android_backend() {}

            std::shared_ptr<device_watcher> create_device_watcher() const override;
        };
    }
}
