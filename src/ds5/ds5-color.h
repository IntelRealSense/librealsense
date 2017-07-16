// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-device.h"

namespace librealsense
{
    class ds5_color : public virtual ds5_device
    {
    public:
        std::shared_ptr<uvc_sensor> create_color_device(const platform::backend& backend,
                                                          const std::vector<platform::uvc_device_info>& all_device_infos);

        ds5_color(const platform::backend& backend,
                  const platform::backend_device_group& group);

    private:
        uint8_t _color_device_idx = -1;
    };
}
