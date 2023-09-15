// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "extension.h"
#include <src/basics.h>

#include <librealsense2/h/rs_sensor.h>
#include <string>


namespace librealsense {


// Anything that can support getting rs2_camera_info should implement this.
//
class info_interface : public virtual recordable< info_interface >
{
public:
    virtual const std::string & get_info( rs2_camera_info info ) const = 0;
    virtual bool supports_info( rs2_camera_info info ) const = 0;

    virtual ~info_interface() = default;
};

MAP_EXTENSION( RS2_EXTENSION_INFO, librealsense::info_interface );


}  // namespace librealsense
