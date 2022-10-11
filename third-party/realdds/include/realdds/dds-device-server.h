// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

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

namespace realdds {

// Forward declaration
namespace topics {
namespace raw {
class device_info;
namespace device {
class notification;
}  // namespace device
}  // namespace raw
class device_info;
}  // namespace topics


class dds_participant;
class dds_publisher;
class dds_stream_server;
class dds_notification_server;
struct image_header;


// Responsible for representing a device at the DDS level.
// Each device has a root path under which the device topics are created, e.g.:
//     realsense/L515/F0090353    <- root
//         /notification
//         /control
//         /stream1
//         /stream2
//         ...
// The device server is meant to manage multiple streams and be able to publish frames to them, while
// also receive instructions (controls) from a client and generate callbacks to the user.
// 
// Streams are named, and are referred to by names, to maintain synchronization (e.g., when starting
// to stream, a notification may be sent, and the metadata may be split to another stream).
//
class dds_device_server
{
public:
    dds_device_server( std::shared_ptr< dds_participant > const & participant, const std::string & topic_root );
    ~dds_device_server();

    // A server is not valid until init() is called with a list of streams that we want to publish.
    // On successful return from init(), each of the streams will be alive so clients will be able
    // to subscribe.
    void init( const std::vector< std::string > & supported_streams_names );

    bool is_valid() const { return( nullptr != _notification_server.get() ); }
    bool operator!() const { return ! is_valid(); }

    void start_streaming( const std::string & stream_name, const image_header & header );

    // `Init` messages are sent when a new reader joins, it holds all required information about the device capabilities (sensors, profiles)
    // Currently it will broadcast the messages to all connected readers (not only the new reader)
    void add_init_msg( topics::raw::device::notification&& notification );
    void publish_image( const std::string& stream_name, const uint8_t* data, size_t size );
    void publish_notification( topics::raw::device::notification&& notification );
    
private:
    std::shared_ptr< dds_participant > _participant;
    std::shared_ptr< dds_publisher > _publisher;
    std::string _topic_root;
    std::unordered_map<std::string, std::shared_ptr<dds_stream_server>> _stream_name_to_server;
    std::shared_ptr< dds_notification_server > _notification_server;
};  // class dds_device_server


}  // namespace realdds
