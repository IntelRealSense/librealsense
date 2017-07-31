// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-device.h"

namespace librealsense
{
    class ds5_color : public virtual ds5_device
    {
    public:
        std::shared_ptr<uvc_sensor> create_color_device(std::shared_ptr<context> ctx,
                                                          const std::vector<platform::uvc_device_info>& all_device_infos);

        ds5_color(std::shared_ptr<context> ctx,
                  const platform::backend_device_group& group);

    private:
        friend class ds5_color_sensor;

        uint8_t _color_device_idx = -1;
        std::shared_ptr<stream_interface> _color_stream;

        lazy<std::vector<uint8_t>> _color_calib_table_raw;
        std::shared_ptr<lazy<rs2_extrinsics>> _color_extrinsic;
    };
}
