// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <unordered_map>
#include <librealsense2/rs.hpp>  // Include RealSense Cross Platform API
#include <librealsense2/utilities/easylogging/easyloggingpp.h>

// Forward declaration
namespace eprosima {
namespace fastdds {
namespace dds {
class DomainParticipant;
class Publisher;
class Topic;
class TypeSupport;
class DataWriter;
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

// Responsible for representing a device at the DDS level.
// Each device has a root path under which the device topics are created, e.g.:
//     realsense/L515/F0090353    <- root
//         /main
//         /control
//         /stream1
//         /stream2
//         ...
// The device server is meant to manage multiple streams and be able to publish frames to them, while
// also receive instructions (controls) from a client and generate callbacks to the user.
class dds_device_server
{
public:
    struct image_header
    {
        int format;
        int height;
        int width;
    };

    dds_device_server( librealsense::dds::dds_participant& participant, const std::string& topic_root );
    ~dds_device_server();

    // A server is not valid until init() is called with a list of streams that we want to publish.
    // On successful return from init(), each of the streams will be alive so clients will be able
    // to subscribe.
    void init( const std::vector<std::string>& supported_streams_names );

    bool is_valid() const { return ( nullptr != _publisher ); }
    bool operator!() const { return ! is_valid(); }
    void set_image_header( const std::string& stream_name, const image_header& header );
    void publish_frame( const std::string& stream_name, uint8_t* frame, int size );
    
private:
    class dds_stream_server;
    
    eprosima::fastdds::dds::DomainParticipant * _participant;
    eprosima::fastdds::dds::Publisher * _publisher;
    std::string _topic_root;
    std::unordered_map<std::string, std::shared_ptr<dds_stream_server>> _stream_name_to_server;
};  // class dds_device_server
}  // namespace dds
}  // namespace librealsense
