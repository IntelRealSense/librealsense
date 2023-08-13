// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <realdds/dds-option.h>
#include <realdds/dds-defines.h>
#include <realdds/dds-trinsics.h>

#include <rsutils/concurrency/concurrency.h>
#include <nlohmann/json_fwd.hpp>

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

    // A server is not valid until init() is called with a list of streams that we want to publish.
    // On successful return from init(), each of the streams will be alive so clients will be able
    // to subscribe.
    void init( const std::vector< std::shared_ptr< dds_stream_server > > & streams,
               const dds_options & options, const extrinsics_map & extr );

    void broadcast( topics::device_info const & );

    bool is_valid() const { return( nullptr != _notification_server.get() ); }
    bool operator!() const { return ! is_valid(); }

    std::map< std::string, std::shared_ptr< dds_stream_server > > const & streams() const { return _stream_name_to_server; }

    void publish_notification( topics::flexible_msg && );
    void publish_metadata( nlohmann::json && );

    bool has_metadata_readers() const;

    typedef std::function< void( const nlohmann::json & msg ) > control_callback;
    void on_open_streams( control_callback callback ) { _open_streams_callback = std::move( callback ); }

    typedef std::function< void( const std::shared_ptr< realdds::dds_option > & option, float value ) > set_option_callback;
    typedef std::function< float( const std::shared_ptr< realdds::dds_option > & option ) > query_option_callback;
    void on_set_option( set_option_callback callback ) { _set_option_callback = std::move( callback ); }
    void on_query_option( query_option_callback callback ) { _query_option_callback = std::move( callback ); }

private:
    void on_control_message_received();
    void handle_control_message( std::string const & id,
                                 nlohmann::json const & control_message,
                                 nlohmann::json & reply );

    void handle_set_option( const nlohmann::json & msg, nlohmann::json & reply );
    void handle_query_option( const nlohmann::json & msg, nlohmann::json & reply );
    std::shared_ptr< dds_option > find_option( const std::string & option_name, const std::string & owner_name );

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

    control_callback _open_streams_callback = nullptr;
    set_option_callback _set_option_callback = nullptr;
    query_option_callback _query_option_callback = nullptr;

    extrinsics_map _extrinsics_map; // <from stream, to stream> to extrinsics
};  // class dds_device_server


}  // namespace realdds
