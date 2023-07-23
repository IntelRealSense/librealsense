// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <realdds/dds-device.h>
#include <realdds/dds-stream-profile.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-option.h>
#include <realdds/topics/device-info-msg.h>
#include <realdds/topics/flexible-msg.h>

#include <fastdds/rtps/common/Guid.h>

#include <nlohmann/json.hpp>

#include <map>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace realdds {


class dds_topic_reader;
class dds_topic_writer;
class dds_subscriber;

namespace topics {
class flexible_msg;
}


class dds_device::impl
{
public:
    topics::device_info const _info;
    dds_guid const _guid;
    std::shared_ptr< dds_participant > const _participant;
    std::shared_ptr< dds_subscriber > _subscriber;

    bool _running = false;

    std::map< std::string, std::shared_ptr< dds_stream > > _streams;

    std::mutex _replies_mutex;
    std::condition_variable _replies_cv;
    std::map< dds_sequence_number, nlohmann::json > _replies;

    std::shared_ptr< dds_topic_reader > _notifications_reader;
    std::shared_ptr< dds_topic_reader > _metadata_reader;
    std::shared_ptr< dds_topic_writer > _control_writer;

    dds_options _options;

    extrinsics_map _extrinsics_map; // <from stream, to stream> to extrinsics

    impl( std::shared_ptr< dds_participant > const & participant,
          dds_guid const & guid,
          topics::device_info const & info );

    void run();
    void open( const dds_stream_profiles & profiles );

    void write_control_message( topics::flexible_msg &&, nlohmann::json * reply = nullptr );

    void set_option_value( const std::shared_ptr< dds_option > & option, float new_value );
    float query_option_value( const std::shared_ptr< dds_option > & option );

    typedef std::function< void( nlohmann::json && md ) > on_metadata_available_callback;
    void on_metadata_available( on_metadata_available_callback cb ) { _on_metadata_available = cb; }

private:
    void create_notifications_reader();
    void create_metadata_reader();
    void create_control_writer();
    bool init();

    struct init_context;
    void handle_device_header( init_context &, nlohmann::json const & );
    void handle_device_options( init_context &, nlohmann::json const & );
    void handle_stream_header( init_context &, nlohmann::json const & );
    void handle_stream_options( init_context &, nlohmann::json const & );

    // notification handlers
    typedef std::map< std::string, void ( dds_device::impl::* )( nlohmann::json const & ) > notification_handlers;
    static notification_handlers const _notification_handlers;
    void handle_notification( nlohmann::json const & );
    void on_option_value( nlohmann::json const & );
    void on_known_notification( nlohmann::json const & );

    on_metadata_available_callback _on_metadata_available = nullptr;

    size_t _message_timeout_ms = 5000;
};


}  // namespace realdds
