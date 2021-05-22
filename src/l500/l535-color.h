// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>
#include <string>
#include <map>

#include "l500-device.h"
#include "stream.h"
#include "l500-depth.h"
#include "algo/thermal-loop/l500-thermal-loop.h"

namespace librealsense
{
    class l535_color :public l500_color
    {
    public:
        l535_color(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);
    };
}
