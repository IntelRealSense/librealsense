// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "d500-device.h"
#include "ds/ds-active-common.h"

namespace librealsense
{
    class d500_active : public virtual d500_device
    {
    public:
        d500_active(std::shared_ptr<context> ctx,
                   const platform::backend_device_group& group);
    private:
        std::shared_ptr<ds_active_common> _ds_active_common;
    };
}
