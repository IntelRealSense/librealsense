// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "rs-dds-sensor-proxy.h"


namespace librealsense {

// For cases when checking if this is< motion_sensor > (like realsense-viewer::subdevice_model)
class dds_motion_sensor_proxy
    : public dds_sensor_proxy
    , public motion_sensor
{
public:
    dds_motion_sensor_proxy( std::string const & sensor_name,
                             software_device * owner,
                             std::shared_ptr< realdds::dds_device > const & dev )
        : dds_sensor_proxy( sensor_name, owner, dev )
    {
    }
};


}  // namespace librealsense
