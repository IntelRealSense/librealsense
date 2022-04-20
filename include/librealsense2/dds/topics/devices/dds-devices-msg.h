// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include "devicesMsgPubSubTypes.h"

namespace librealsense {
namespace dds {
namespace topics_phys {
struct device_info
{
    std::string name;
    std::string serial;
    std::string product_line;
    bool locked;

    device_info()
        : name()
        , serial()
        , product_line()
        , locked( false )
    {
    }
};

}  // namespace topics_phys
}  // namespace dds
}  // namespace librealsense
