// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>

#include "device.h"
#include "image.h"
#include "metadata-parser.h"

#include "d500-active.h"
#include "d500-private.h"
#include "d500-info.h"
#include "ds/ds-options.h"
#include "ds/ds-timestamp.h"

namespace librealsense
{
    d500_active::d500_active( std::shared_ptr< const d500_info > const & dev_info )
    : device( dev_info )
    , d500_device( dev_info )
    {
        using namespace ds;

        _ds_active_common = std::make_shared<ds_active_common>(get_raw_depth_sensor(), get_depth_sensor(), this,
            _device_capabilities, _hw_monitor, _fw_version);

        _ds_active_common->register_options();
    }
}
