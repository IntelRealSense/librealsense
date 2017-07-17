// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-device.h"

namespace librealsense
{
    class ds5_active : public virtual ds5_device
    {
    public:
        ds5_active(const std::shared_ptr<context>& ctx,
                   const platform::backend_device_group& group);

        std::shared_ptr<matcher> create_matcher(rs2_stream stream) const override;
       
    };
}
