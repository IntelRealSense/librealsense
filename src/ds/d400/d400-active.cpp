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
#include "d400-info.h"
#include "ds/ds-timestamp.h"

namespace librealsense
{
    d400_active::d400_active( std::shared_ptr< const d400_info > const & dev_info )
        : device(dev_info), d400_device(dev_info)
    {
        using namespace ds;

        //Projector's capacity is established based on actual HW capabilities
        _ds_active_common = std::make_shared<ds_active_common>(get_raw_depth_sensor(), get_depth_sensor(), this, 
            _device_capabilities, _hw_monitor, _fw_version);

        _ds_active_common->register_options();
    }
}
