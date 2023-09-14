// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "rs-dds-sensor-proxy.h"


namespace librealsense {

// For cases when checking if this is< depth_sensor > (like realsense-viewer::subdevice_model)
class dds_depth_sensor_proxy
    : public dds_sensor_proxy
    , public depth_sensor
{
public:
    dds_depth_sensor_proxy( std::string const & sensor_name,
                            software_device * owner,
                            std::shared_ptr< realdds::dds_device > const & dev )
        : dds_sensor_proxy( sensor_name, owner, dev )
    {
    }

    // Needed by abstract interfaces
    float get_depth_scale() const override { return get_option( RS2_OPTION_DEPTH_UNITS ).query(); }
};


}  // namespace librealsense
