// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include <sstream>
#include <memory>
#include "mipi-device-info.h"

namespace librealsense {
namespace platform {

class mipi_device;
typedef std::shared_ptr<mipi_device> rs_mipi_device;

class mipi_device
{
public:
    mipi_device(const mipi_device_info& info) : _info(info) {}
    virtual ~mipi_device() = default;
    static rs_mipi_device create_mipi_device(const mipi_device_info& info)
    {
        return std::make_shared<mipi_device>(info);
    }
    const mipi_device_info get_info() const { return _info; };
private:
    const mipi_device_info _info;
};


}  // namespace platform
}  // namespace librealsense
