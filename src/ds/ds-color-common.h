// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "device.h"
#include "core/video.h"
#include "ds-device-common.h"

namespace librealsense
{
    class ds_color_common
    {
    public:
        ds_color_common( const std::shared_ptr< uvc_sensor > & raw_color_ep,
            synthetic_sensor& color_ep,
            firmware_version fw_version,
            std::shared_ptr<hw_monitor> hw_monitor,
            device* owner);
        void register_color_options();
        void register_standard_options();
        void register_metadata();

    private:
        std::shared_ptr< uvc_sensor > _raw_color_ep;
        synthetic_sensor& _color_ep;
        firmware_version _fw_version;
        std::shared_ptr<hw_monitor> _hw_monitor;
        device* _owner;
    };
}
