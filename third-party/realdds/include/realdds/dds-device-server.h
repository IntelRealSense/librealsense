// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/utilities/concurrency/concurrency.h>
#include <third-party/json.hpp>

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace realdds {


// Forward declaration
namespace topics {
class flexible_msg;
class device_info;
namespace raw {
class device_info;
}  // namespace raw
}  // namespace topics


class dds_participant;
class dds_publisher;
class dds_subscriber;
class dds_stream_server;
class dds_notification_server;
class dds_topic_reader;
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
    void init( const std::vector< std::shared_ptr< dds_stream_server > > & streams );

    bool is_valid() const { return( nullptr != _notification_server.get() ); }
    bool operator!() const { return ! is_valid(); }

    void start_streaming( const std::vector< std::pair < std::string, image_header > > & ); //< stream_name, header > pairs
    void stop_streaming( const std::vector< std::string > & stream_to_close );

    void publish_image( const std::string & stream_name, const uint8_t * data, size_t size );
    void publish_notification( topics::flexible_msg && );
    
    typedef std::function< void( nlohmann::json msg, dds_device_server * server ) > control_callback;
    void on_open_streams( control_callback callback ) { _open_streams_callback = std::move( callback ); }
    void on_close_streams( control_callback callback ) { _close_streams_callback = std::move( callback ); }

private:
    void on_control_message_received();
    void handle_control_message( topics::flexible_msg control_message );

    std::shared_ptr< dds_publisher > _publisher;
    std::shared_ptr< dds_subscriber > _subscriber;
    std::string _topic_root;
    std::unordered_map<std::string, std::shared_ptr<dds_stream_server>> _stream_name_to_server;
    std::shared_ptr< dds_notification_server > _notification_server;
    std::shared_ptr< dds_topic_reader > _control_reader;
    dispatcher _control_dispatcher;
    control_callback _open_streams_callback = nullptr;
    control_callback _close_streams_callback = nullptr;
};  // class dds_device_server


}  // namespace realdds
