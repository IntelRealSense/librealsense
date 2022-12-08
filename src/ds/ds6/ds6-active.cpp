// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>

#include "device.h"
#include "context.h"
#include "image.h"
#include "metadata-parser.h"

#include "ds6-active.h"
#include "ds6-private.h"
#include "ds/ds-options.h"
#include "ds/ds-timestamp.h"

namespace librealsense
{
    ds6_active::ds6_active(std::shared_ptr<context> ctx,
                           const platform::backend_device_group& group)
        : device(ctx, group), ds6_device(ctx, group)
    {
        using namespace ds;

        _ds_active_common = std::make_shared<ds_active_common>(get_raw_depth_sensor(), get_depth_sensor(), this,
            _device_capabilities);

        _ds_active_common->register_options();
    }
}
