// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/rs.hpp>  // Include RealSense Cross Platform API

// Forward declaration
namespace eprosima {
namespace fastdds {
namespace dds {
class DomainParticipant;
class Publisher;
class Topic;
class TypeSupport;
}  // namespace dds
}  // namespace fastdds
}  // namespace eprosima

namespace librealsense {
namespace dds {

// Forward declaration
namespace topics {
namespace raw {
class device_info;
}  // namespace raw
class device_info;
}  // namespace topics


class dds_participant;

class dds_device_server
{
public:
    dds_device_server( librealsense::dds::dds_participant & participant, const std::string &topic_root );
    ~dds_device_server();
    bool run();
    void publish_dds_video_frame(const std::string& topic_name, uint8_t* frame );

private:
    bool create_dds_publisher( const std::string& stream_name );
    eprosima::fastdds::dds::DomainParticipant * _participant;
    eprosima::fastdds::dds::Publisher * _publisher;
    eprosima::fastdds::dds::Topic * _topic;
    std::shared_ptr<eprosima::fastdds::dds::TypeSupport> _topic_type_ptr;
    std::string _topic_root;
    bool _init_ok;
};  // class dds_device_server
}  // namespace dds
}  // namespace librealsense
