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

#include <rsutils/signal.h>
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
    enum class state_t
    {
        WAIT_FOR_DEVICE_HEADER,
        WAIT_FOR_DEVICE_OPTIONS,
        WAIT_FOR_STREAM_HEADER,
        WAIT_FOR_STREAM_OPTIONS,
        READY
    };
    static char const * to_string( state_t );
    void set_state( state_t );

    state_t _state = state_t::WAIT_FOR_DEVICE_HEADER;
    size_t _n_streams_expected = 0;  // needed only until ready

public:
    topics::device_info const _info;
    nlohmann::json const _device_settings;
    dds_guid _server_guid;
    std::shared_ptr< dds_participant > const _participant;
    std::shared_ptr< dds_subscriber > _subscriber;

    std::map< std::string, std::shared_ptr< dds_stream > > _streams;

    std::mutex _replies_mutex;
    std::condition_variable _replies_cv;
    std::map< dds_sequence_number, nlohmann::json > _replies;
    size_t _reply_timeout_ms;

    std::shared_ptr< dds_topic_reader > _notifications_reader;
    std::shared_ptr< dds_topic_reader > _metadata_reader;
    std::shared_ptr< dds_topic_writer > _control_writer;

    dds_options _options;

    extrinsics_map _extrinsics_map; // <from stream, to stream> to extrinsics

    impl( std::shared_ptr< dds_participant > const & participant,
          topics::device_info const & info );

    dds_guid const & guid() const;
    std::string debug_name() const;

    void wait_until_ready( size_t timeout_ms );
    bool is_ready() const { return state_t::READY == _state; }

    void open( const dds_stream_profiles & profiles );

    void write_control_message( topics::flexible_msg &&, nlohmann::json * reply = nullptr );

    void set_option_value( const std::shared_ptr< dds_option > & option, float new_value );
    float query_option_value( const std::shared_ptr< dds_option > & option );

    using on_metadata_available_signal = rsutils::signal< std::shared_ptr< const nlohmann::json > const & >;
    using on_metadata_available_callback = on_metadata_available_signal::callback;
    rsutils::subscription on_metadata_available( on_metadata_available_callback && cb )
    {
        return _on_metadata_available.subscribe( std::move( cb ) );
    }

    using on_device_log_signal = rsutils::signal< dds_time const &,          // timestamp
                                                  char,                      // type
                                                  std::string const &,       // text
                                                  nlohmann::json const & >;  // data
    using on_device_log_callback = on_device_log_signal::callback;
    rsutils::subscription on_device_log( on_device_log_callback && cb )
    {
        return _on_device_log.subscribe( std::move( cb ) );
    }

    using on_notification_signal = rsutils::signal< std::string const &, nlohmann::json const & >;
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
    void on_option_value( nlohmann::json const &, eprosima::fastdds::dds::SampleInfo const & );
    void on_known_notification( nlohmann::json const &, eprosima::fastdds::dds::SampleInfo const & );
    void on_log( nlohmann::json const &, eprosima::fastdds::dds::SampleInfo const & );
    void on_device_header( nlohmann::json const &, eprosima::fastdds::dds::SampleInfo const & );
    void on_device_options( nlohmann::json const &, eprosima::fastdds::dds::SampleInfo const & );
    void on_stream_header( nlohmann::json const &, eprosima::fastdds::dds::SampleInfo const & );
    void on_stream_options( nlohmann::json const &, eprosima::fastdds::dds::SampleInfo const & );

    typedef std::map< std::string,
                      void ( dds_device::impl::* )( nlohmann::json const &,
                                                    eprosima::fastdds::dds::SampleInfo const & ) >
        notification_handlers;
    static notification_handlers const _notification_handlers;
    void handle_notification( nlohmann::json const &, eprosima::fastdds::dds::SampleInfo const & );

    on_metadata_available_signal _on_metadata_available;
    on_device_log_signal _on_device_log;
    on_notification_signal _on_notification;
};


}  // namespace realdds
