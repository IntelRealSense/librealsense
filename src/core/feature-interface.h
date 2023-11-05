// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "extension.h"

#include <librealsense2/h/rs_sensor.h>
#include <string>


namespace librealsense {


// Anything that can support getting rs2_feature should implement this.
//
class feature_interface
{
public:
    virtual bool supports_feature( rs2_feature feature ) const = 0;

    virtual ~feature_interface() = default;
};

MAP_EXTENSION( RS2_EXTENSION_FEATURE, librealsense::feature_interface );


}  // namespace librealsense
