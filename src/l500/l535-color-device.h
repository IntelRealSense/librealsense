// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "l500-color.h"

namespace librealsense {
namespace ivcam2 {
namespace l535 {


class color_device : public l500_color
{
public:
    color_device( std::shared_ptr< context > ctx, const platform::backend_device_group & group );
};


}  // namespace l535
}  // namespace ivcam2
}  // namespace librealsense
