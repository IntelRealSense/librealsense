// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "extension.h"
#include <string>

namespace librealsense
{
    class info_interface : public virtual recordable<info_interface>
    {
    public:
        virtual const std::string& get_info(rs2_camera_info info) const = 0;
        virtual bool supports_info(rs2_camera_info info) const = 0;

        virtual ~info_interface() = default;
    };
}
