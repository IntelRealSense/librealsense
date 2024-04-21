// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "device.h"
#include "core/video.h"
#include "ds/ds-device-common.h"
#include "ds-private.h"

namespace librealsense
{
    class backend_device;

    class ds_active_common
    {
    public:
        ds_active_common( const std::shared_ptr< uvc_sensor > & raw_depth_ep,
                          synthetic_sensor & depth_ep,
                          backend_device * owner,
                          ds::ds_caps device_capabilities,
                          std::shared_ptr< hw_monitor > hw_monitor,
                          firmware_version firmware_version );
        void register_options();

    private:
        std::shared_ptr< uvc_sensor > _raw_depth_ep;
        synthetic_sensor& _depth_ep;
        backend_device* _owner;
        ds::ds_caps _device_capabilities;
        std::shared_ptr<hw_monitor> _hw_monitor;
        firmware_version _fw_version;
    };

    

}
