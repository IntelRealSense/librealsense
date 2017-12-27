// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "core/streaming.h"
#include "core/video.h"
#include "device.h"

namespace librealsense
{
    class bypass_device : public device
    {
    public:
        bypass_device() 
            : device(std::make_shared<context>(backend_type::standard), {})
        {

        }
    };

    class bypass_sensor : public sensor_base
    {
    public:
        bypass_sensor(std::string name, bypass_device* owner) 
            : sensor_base(name, owner)
        {

        }

    };
}
