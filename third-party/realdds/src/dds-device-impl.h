// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <realdds/dds-device.h>
#include <realdds/dds-stream-profile.h>
#include <realdds/dds-utilities.h>

#include <realdds/topics/device-info/device-info-msg.h>
#include <realdds/topics/notification/notification-msg.h>
#include <fastdds/rtps/common/Guid.h>

#include <map>
#include <memory>
#include <atomic>


namespace realdds {


class dds_topic_reader;
class dds_topic_writer;


class dds_device::impl
{
public:
    topics::device_info const _info;
    dds_guid const _guid;
    std::shared_ptr< dds_participant > const _participant;

    bool _running = false;

    std::map< std::string, std::shared_ptr< dds_stream > > _streams;
    std::atomic<uint32_t> _control_message_counter = { 0 };

    std::shared_ptr< dds_topic_reader > _notifications_reader;
    std::shared_ptr< dds_topic_writer > _control_writer;

    impl( std::shared_ptr< dds_participant > const & participant,
          dds_guid const & guid,
          topics::device_info const & info );

    void run();

    bool write_control_message( void * msg );

private:
    void create_notifications_reader();
    void create_control_writer();
    bool init();
};


}  // namespace realdds
