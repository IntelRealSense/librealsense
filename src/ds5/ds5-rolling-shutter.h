// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-device.h"

namespace librealsense
{
    class ds5_rolling_shutter : public virtual ds5_device
    {
    public:
        ds5_rolling_shutter(std::shared_ptr<context> ctx,
                            const platform::backend_device_group& group);
    };
}
