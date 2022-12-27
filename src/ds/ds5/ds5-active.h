// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-device.h"
#include "ds/ds-active-common.h"

namespace librealsense
{
    // Active means the HW includes an active projector
    class ds5_active : public virtual ds5_device
    {
    public:
        ds5_active(std::shared_ptr<context> ctx,
                   const platform::backend_device_group& group);
    private:
        std::shared_ptr<ds_active_common> _ds_active_common;
    };
}
