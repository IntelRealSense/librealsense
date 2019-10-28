// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "../rsusb-backend/rsusb-backend.h"

namespace librealsense
{
    namespace platform
    {
        class rs_backend_linux : public rs_backend
        {
        public:
            rs_backend_linux() {}
            std::shared_ptr<device_watcher> create_device_watcher() const override;
        };
    }
}
