// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include <string>
#include <memory>


namespace librealsense {
namespace dds {


class dds_topic;
class dds_participant;


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

    static std::shared_ptr< dds_topic > create( std::shared_ptr< dds_participant > const & participant,
                                                char const * topic_name );

    std::string name;
    std::string serial;
    std::string product_line;
    std::string topic_root;
    bool locked = false;
};


}  // namespace topics
}  // namespace dds
}  // namespace librealsense
