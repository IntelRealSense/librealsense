// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <realdds/dds-device.h>
#include <realdds/dds-stream-profile.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-option.h>
#include <realdds/topics/device-info/device-info-msg.h>
#include <realdds/topics/flexible/flexible-msg.h>

#include <fastdds/rtps/common/Guid.h>

#include <third-party/json.hpp>

#include <map>
#include <memory>
#include <atomic>
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

    std::atomic<uint32_t> _control_message_counter = { 0 };
    std::mutex _option_mutex;
    std::condition_variable _option_cv;
    std::queue< nlohmann::json > _option_response_queue;

    std::shared_ptr< dds_topic_reader > _notifications_reader;
    std::shared_ptr< dds_topic_writer > _control_writer;

    dds_options _options;

    impl( std::shared_ptr< dds_participant > const & participant,
          dds_guid const & guid,
          topics::device_info const & info );

    void run( size_t message_timeout_ms );
    void open( const dds_stream_profiles & profiles );
    void close( const dds_streams & streams );

    void write_control_message( topics::flexible_msg && );

    void set_option_value( const std::shared_ptr< dds_option > & option, float new_value );
    float query_option_value( const std::shared_ptr< dds_option > & option );

private:
    void create_notifications_reader();
    void create_control_writer();
    bool init();

    size_t _message_timeout_ms = 5000;
};


}  // namespace realdds
