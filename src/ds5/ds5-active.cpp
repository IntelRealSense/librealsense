// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>

#include "device.h"
#include "context.h"
#include "image.h"
#include "metadata-parser.h"

#include "ds5-active.h"
#include "ds5-private.h"
#include "ds5-options.h"
#include "ds5-timestamp.h"

namespace librealsense
{
    ds5_active::ds5_active(const platform::backend& backend,
                           const platform::backend_device_group& group)
        : ds5_device(backend, group)
    {
        using namespace ds;

        get_depth_sensor().register_option(RS2_OPTION_EMITTER_ENABLED, std::make_shared<emitter_option>(get_depth_sensor()));

        get_depth_sensor().register_option(RS2_OPTION_LASER_POWER,
            std::make_shared<uvc_xu_option<uint16_t>>(get_depth_sensor(),
                depth_xu,
                DS5_LASER_POWER, "Manual laser power in mw. applicable only when laser power mode is set to Manual"));

        get_depth_sensor().register_option(RS2_OPTION_PROJECTOR_TEMPERATURE,
                                 std::make_shared<asic_and_projector_temperature_options>(get_depth_sensor(),
                                                                                          RS2_OPTION_PROJECTOR_TEMPERATURE));
    }
}
