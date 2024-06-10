// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <realdds/dds-option.h>
#include <realdds/dds-defines.h>
#include <realdds/dds-time.h>
#include <realdds/dds-trinsics.h>

#include <rsutils/concurrency/concurrency.h>
#include <rsutils/json-fwd.h>
#include <rsutils/string/slice.h>

#include <map>
#include <vector>
#include <memory>
#include <string>
#include <functional>


namespace eprosima {
namespace fastdds {
namespace dds {
struct SampleInfo;
}  // namespace dds
}  // namespace fastdds
}  // namespace eprosima


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
class dds_topic_writer;
class dds_device_broadcaster;
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

    dds_guid const & guid() const;
    std::shared_ptr< dds_participant > participant() const;
    std::shared_ptr< dds_subscriber > subscriber() const { return _subscriber; }
    std::string const & topic_root() const { return _topic_root; }
    rsutils::string::slice debug_name() const;

    // A server is not valid until init() is called with a list of streams that we want to publish.
    // On successful return from init(), each of the streams will be alive so clients will be able
    // to subscribe.
    void init( const std::vector< std::shared_ptr< dds_stream_server > > & streams,
               const dds_options & options, const extrinsics_map & extr );

    // After initialization, the device can be broadcast on the device-info topic
    void broadcast( topics::device_info const & );

    // Once broadcast, we can also broadcast that we expect the device to go down:
    // To wait for acknowledgements, pass in a timeout; the return value will be false if a timeout occurs.
    bool broadcast_disconnect( dds_time ack_timeout = {} );

    bool is_valid() const { return( nullptr != _notification_server.get() ); }
    bool operator!() const { return ! is_valid(); }

    std::map< std::string, std::shared_ptr< dds_stream_server > > const & streams() const { return _stream_name_to_server; }

    void publish_notification( topics::flexible_msg && );
    void publish_metadata( rsutils::json && );

    bool has_metadata_readers() const;

    typedef std::function< void( const std::shared_ptr< realdds::dds_option > & option, rsutils::json & value ) > set_option_callback;
    typedef std::function< rsutils::json( const std::shared_ptr< realdds::dds_option > & option ) > query_option_callback;
    void on_set_option( set_option_callback callback ) { _set_option_callback = std::move( callback ); }
    void on_query_option( query_option_callback callback ) { _query_option_callback = std::move( callback ); }

    typedef std::function< bool( std::string const &, rsutils::json const &, rsutils::json & ) > control_callback;
    void on_control( control_callback callback ) { _control_callback = std::move( callback ); }

    // Locate an option based on stream name (empty for device option) and option name
    std::shared_ptr< dds_option > find_option( std::string const & option_name, std::string const & stream_name ) const;
    // Same as find_options, except throws if not found
    std::shared_ptr< dds_option > get_option( std::string const & option_name, std::string const & stream_name ) const;

private:
    struct control_sample;

    void on_control_message_received();
    void on_set_option( control_sample const &, rsutils::json & reply );
    void on_query_option( control_sample const &, rsutils::json & reply );
    void on_query_options( control_sample const &, rsutils::json & reply );

    rsutils::json query_option( std::shared_ptr< dds_option > const & ) const;

    std::shared_ptr< dds_publisher > _publisher;
    std::shared_ptr< dds_subscriber > _subscriber;
    std::string _topic_root;
    std::map< std::string, std::shared_ptr< dds_stream_server > > _stream_name_to_server;
    dds_options _options;
    std::shared_ptr< dds_notification_server > _notification_server;
    std::shared_ptr< dds_topic_reader > _control_reader;
    std::shared_ptr< dds_topic_writer > _metadata_writer;
    std::shared_ptr< dds_device_broadcaster > _broadcaster;
    dispatcher _control_dispatcher;

    set_option_callback _set_option_callback;
    query_option_callback _query_option_callback;
    control_callback _control_callback;

    extrinsics_map _extrinsics_map; // <from stream, to stream> to extrinsics
};  // class dds_device_server


}  // namespace realdds
