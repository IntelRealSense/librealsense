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

#include "d400-active.h"
#include "d400-private.h"
#include "d400-options.h"
#include "ds/ds-timestamp.h"

namespace librealsense
{
    d400_active::d400_active(std::shared_ptr<context> ctx,
                           const platform::backend_device_group& group)
        : device(ctx, group), d400_device(ctx, group)
    {
        using namespace ds;

        //Projector's capacity is established based on actual HW capabilities
        _ds_active_common = std::make_shared<ds_active_common>(get_raw_depth_sensor(), get_depth_sensor(), this, 
            _device_capabilities, _hw_monitor, _fw_version);

        _ds_active_common->register_options();
    }
}
