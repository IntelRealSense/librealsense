// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include "deviceInfoPubSubTypes.h"

namespace librealsense {
namespace dds {
namespace topics {
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

    device_info( const topics::raw::device_info & dev )
        : name( dev.name().data() )
        , serial( dev.serial_number().data() )
        , product_line( dev.product_line().data() )
        , locked( dev.locked() )

    {
    }
};

}  // namespace topics
}  // namespace dds
}  // namespace librealsense
