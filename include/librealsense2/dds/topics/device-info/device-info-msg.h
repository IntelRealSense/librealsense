// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include "deviceInfoPubSubTypes.h"

namespace librealsense {
namespace dds {
namespace topics {
class device_info
{
public:
    using type = raw::device_infoPubSubType;
    static constexpr char const * TOPIC_NAME = "realsense/device-info";

    device_info() = default;

    device_info( const raw::device_info & dev )
        : name( dev.name().data() )
        , serial( dev.serial_number().data() )
        , product_line( dev.product_line().data() )
        , topic_root( dev.topic_root().data() )
        , locked( dev.locked() )
    {
    }

    std::string name;
    std::string serial;
    std::string product_line;
    std::string topic_root;
    bool locked = false;
};

}  // namespace topics
}  // namespace dds
}  // namespace librealsense
