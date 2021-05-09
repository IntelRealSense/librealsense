// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once
#include "hw-monitor.h"
#include "l500-device.h"

namespace librealsense {
namespace ivcam2 {
namespace l535 {

    class device_options : public virtual l500_device
    {
    public:
        device_options( std::shared_ptr< context > ctx, const platform::backend_device_group & group );
    };

}  // namespace l535
}  // namespace ivcam2
}  // namespace librealsense
