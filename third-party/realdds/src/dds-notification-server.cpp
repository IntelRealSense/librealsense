// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-notification-server.h>

#include <realdds/dds-participant.h>
#include <realdds/dds-publisher.h>
#include <realdds/dds-stream-server.h>
#include <realdds/dds-utilities.h>
#include <realdds/topics/dds-topics.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-topic-writer.h>

#include <fastdds/dds/topic/Topic.hpp>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <librealsense2/utilities/concurrency/concurrency.h>


using namespace eprosima::fastdds::dds;

namespace realdds {


dds_notification_server::dds_notification_server( std::shared_ptr< dds_publisher > const & publisher, const std::string & topic_name )
    : _publisher( publisher )
    , _notifications_loop( [this]( dispatcher::cancellable_timer timer ) {
        if( _active )
        {
            // We can send 2 types of notifications:
            // * Initialize notifications that should be sent for every new reader (like sensors & profiles data)
            // * Instant notifications that can be sent upon an event and is not required to be
            //   resent to new readers
            std::unique_lock< std::mutex > lock( _notification_send_mutex );
            _send_notification_cv.wait( lock, [this]() {
                return ! _active || _send_init_msgs || _new_instant_notification;
            } );

            if( _active && _send_init_msgs )
            {
                send_discovery_notifications();
                _send_init_msgs = false;
            }

            if( _active && _new_instant_notification )
            {
                // Send all instant notifications
                topics::raw::notification notification;
                constexpr unsigned timeout_in_ms = 1000;
                while( _instant_notifications.dequeue( &notification, timeout_in_ms ) )
                {
                    DDS_API_CALL( _writer->get()->write( &notification ) );
                }
                _new_instant_notification = false;
            }
        }
    } )
{
    auto topic = topics::notification::create_topic( publisher->get_participant(), topic_name.c_str() );
    _writer = std::make_shared< dds_topic_writer >( topic, publisher );

    _writer->on_publication_matched( [this]( PublicationMatchedStatus const & info ) {
        if( info.current_count_change == 1 )
        {
            {
                std::lock_guard< std::mutex > lock( _notification_send_mutex );
                _send_init_msgs = true;
            }
            _send_notification_cv.notify_all();
        }
        else if( info.current_count_change == -1 )
        {
        }
        else
        {
            LOG_ERROR( std::to_string( info.current_count_change )
                       << " is not a valid value for on_publication_matched" );
        }
    } );

    dds_topic_writer::qos wqos( RELIABLE_RELIABILITY_QOS );
    wqos.history().depth = 10;                                                                      // default is 1
    wqos.endpoint().history_memory_policy = eprosima::fastrtps::rtps::DYNAMIC_RESERVE_MEMORY_MODE;  // TODO: why?
    _writer->run( wqos );

    _notifications_loop.start();
    _active = true;
}


dds_notification_server::~dds_notification_server()
{
    // Mark us inactive and wake up the active object so we can properly stop it
    _active = false;
    _send_notification_cv.notify_all();
    _notifications_loop.stop();
}


void dds_notification_server::send_notification( topics::notification && notification )
{
    topics::raw::notification raw_notification;
    raw_notification.data_type( (topics::raw::notification_data_type)notification._data_type );
    raw_notification.version()[0] = notification._version >> 24 & 0xFF;
    raw_notification.version()[1] = notification._version >> 16 & 0xFF;
    raw_notification.version()[2] = notification._version >>  8 & 0xFF;
    raw_notification.version()[3] = notification._version       & 0xFF;
    raw_notification.data( std::move( notification._data ) );

    std::unique_lock< std::mutex > lock( _notification_send_mutex );
    if( ! _instant_notifications.enqueue( std::move( raw_notification ) ) )
        LOG_ERROR( "error while trying to enqueue a notification" );
};


void dds_notification_server::add_discovery_notification( topics::notification && notification )
{
    topics::raw::notification raw_notification;
    raw_notification.data_type( (topics::raw::notification_data_type) notification._data_type );
    raw_notification.version()[0] = notification._version >> 24 & 0xFF;
    raw_notification.version()[1] = notification._version >> 16 & 0xFF;
    raw_notification.version()[2] = notification._version >> 8 & 0xFF;
    raw_notification.version()[3] = notification._version & 0xFF;
    raw_notification.data( std::move( notification._data ) );

    std::unique_lock< std::mutex > lock( _notification_send_mutex );
    _discovery_notifications.push_back( std::move( raw_notification ) );
};


void dds_notification_server::send_discovery_notifications()
{
    // Send all initialization notifications
    LOG_DEBUG( "broadcasting discovery notifications" );
    for( auto notification : _discovery_notifications )
    {
        DDS_API_CALL( _writer->get()->write( &notification ) );
    }
}


}  // namespace realdds
