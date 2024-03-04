// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <realdds/dds-device.h>
#include <realdds/dds-stream-profile.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-option.h>
#include <realdds/topics/device-info-msg.h>
#include <realdds/topics/flexible-msg.h>

#include <fastdds/rtps/common/Guid.h>

#include <rsutils/signal.h>
#include <rsutils/json.h>

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
    enum class state_t
    {
        OFFLINE,                  // disconnected by device-watcher
        ONLINE,                   // default state, waiting for handshake (device-header)
        WAIT_FOR_DEVICE_OPTIONS,  //   |
        WAIT_FOR_STREAM_HEADER,   //   | handshake (initialization)
        WAIT_FOR_STREAM_OPTIONS,  //   |
        READY                     // post handshake; streamable, controllable, etc.
    };

    void set_state( state_t );

    state_t _state = state_t::ONLINE;
    size_t _n_streams_expected = 0;  // needed only until ready

    topics::device_info _info;
    rsutils::json const _device_settings;
    dds_guid _server_guid;
    std::shared_ptr< dds_participant > const _participant;
    std::shared_ptr< dds_subscriber > _subscriber;

    std::map< std::string, std::shared_ptr< dds_stream > > _streams;

    std::mutex _replies_mutex;
    std::condition_variable _replies_cv;
    std::map< dds_sequence_number, rsutils::json > _replies;
    size_t const _reply_timeout_ms;

    std::shared_ptr< dds_topic_reader > _notifications_reader;
    std::shared_ptr< dds_topic_reader > _metadata_reader;
    std::shared_ptr< dds_topic_writer > _control_writer;

    dds_options _options;

    extrinsics_map _extrinsics_map; // <from stream, to stream> to extrinsics

    impl( std::shared_ptr< dds_participant > const & participant,
          topics::device_info const & info );

    void reset();

    dds_guid const & guid() const;
    std::string debug_name() const;

    bool is_offline() const { return state_t::OFFLINE == _state; }
    bool is_online() const { return ! is_offline(); }
    bool is_ready() const { return state_t::READY == _state; }

    void open( const dds_stream_profiles & profiles );

    void write_control_message( topics::flexible_msg &&, rsutils::json * reply = nullptr );

    void set_option_value( const std::shared_ptr< dds_option > & option, rsutils::json new_value );
    rsutils::json query_option_value( const std::shared_ptr< dds_option > & option );

    using on_metadata_available_signal = rsutils::signal< std::shared_ptr< const rsutils::json > const & >;
    using on_metadata_available_callback = on_metadata_available_signal::callback;
    rsutils::subscription on_metadata_available( on_metadata_available_callback && cb )
    {
        return _on_metadata_available.subscribe( std::move( cb ) );
    }

    using on_device_log_signal = rsutils::signal< dds_nsec,                  // timestamp
                                                  char,                      // type
                                                  std::string const &,       // text
                                                  rsutils::json const & >;   // data
    using on_device_log_callback = on_device_log_signal::callback;
    rsutils::subscription on_device_log( on_device_log_callback && cb )
    {
        return _on_device_log.subscribe( std::move( cb ) );
    }

    using on_notification_signal = rsutils::signal< std::string const &, rsutils::json const & >;
    using on_notification_callback = on_notification_signal::callback;
    rsutils::subscription on_notification( on_notification_callback && cb )
    {
        return _on_notification.subscribe( std::move( cb ) );
    }

private:
    void create_notifications_reader();
    void create_metadata_reader();
    void create_control_writer();

    // notification handlers
    void on_set_option( rsutils::json const &, eprosima::fastdds::dds::SampleInfo const & );
    void on_query_options( rsutils::json const &, eprosima::fastdds::dds::SampleInfo const & );
    void on_known_notification( rsutils::json const &, eprosima::fastdds::dds::SampleInfo const & );
    void on_log( rsutils::json const &, eprosima::fastdds::dds::SampleInfo const & );
    void on_device_header( rsutils::json const &, eprosima::fastdds::dds::SampleInfo const & );
    void on_device_options( rsutils::json const &, eprosima::fastdds::dds::SampleInfo const & );
    void on_stream_header( rsutils::json const &, eprosima::fastdds::dds::SampleInfo const & );
    void on_stream_options( rsutils::json const &, eprosima::fastdds::dds::SampleInfo const & );

    void on_notification( rsutils::json &&, eprosima::fastdds::dds::SampleInfo const & );

    on_metadata_available_signal _on_metadata_available;
    on_device_log_signal _on_device_log;
    on_notification_signal _on_notification;
};


}  // namespace realdds
