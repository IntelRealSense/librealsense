// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds6-device.h"
#include "ds/ds-active-common.h"

namespace librealsense
{
    class ds6_active : public virtual ds6_device
    {
    public:
        ds6_active(std::shared_ptr<context> ctx,
                   const platform::backend_device_group& group);
    private:
        std::shared_ptr<ds_active_common> _ds_active_common;
    };
}
