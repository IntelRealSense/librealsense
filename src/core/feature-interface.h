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
    // Features that can be queried if supported.
    // Some features may be available only on some models or as of specific FW version.
    enum class feature
    {
        AUTO_EXPOSURE_ROI,  // Auto exposure can be optimized for set region of interest
        HDR,                // Combine images of varying exposure value into a single image for better visibility
        EMITTER_FREQUENCY,  // Control laser emitter frequency
        AMPLITUDE_FACTOR,   // Advanced mode. Can set disparity modulation amplitude factor value
        REMOVE_IR_PATTERN,  // Advanced mode. Can use "remove IR pattern" preset
    };

    virtual bool supports_feature( feature f ) const = 0;

    virtual ~feature_interface() = default;
};


}  // namespace librealsense
