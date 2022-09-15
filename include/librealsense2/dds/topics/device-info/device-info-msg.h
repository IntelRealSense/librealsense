// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>

namespace librealsense {
namespace dds {
namespace topics {
namespace raw {
class device_infoPubSubType;
class device_info;
}


class device_info
{
public:
    using type = raw::device_infoPubSubType;
    static constexpr char const * TOPIC_NAME = "realsense/device-info";

    device_info() = default;
    device_info( const raw::device_info & );

    std::string name;
    std::string serial;
    std::string product_line;
    std::string topic_root;
    bool locked = false;
};


}  // namespace topics
}  // namespace dds
}  // namespace librealsense
