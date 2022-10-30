// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-device-server.h>

#include <realdds/dds-participant.h>
#include <realdds/dds-publisher.h>
#include <realdds/dds-subscriber.h>
#include <realdds/dds-stream-server.h>
#include <realdds/dds-stream-profile.h>
#include <realdds/dds-notification-server.h>
#include <realdds/dds-control-server.h>
#include <realdds/dds-utilities.h>
#include <realdds/topics/device-info/device-info-msg.h>
#include <realdds/topics/image/image-msg.h>
#include <realdds/topics/notification/notification-msg.h>
#include <realdds/topics/control/control-msg.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-topic-writer.h>

#include <fastdds/dds/topic/Topic.hpp>


using namespace eprosima::fastdds::dds;
using namespace realdds;


dds_device_server::dds_device_server( std::shared_ptr< dds_participant > const & participant,
                                      const std::string & topic_root )
    : _publisher( std::make_shared< dds_publisher >( participant ))
    , _subscriber( std::make_shared< dds_subscriber >( participant ))
    , _topic_root( topic_root )
{
    LOG_DEBUG( "device server created @ '" << _topic_root << "'" );
}


dds_device_server::~dds_device_server()
{
    _stream_name_to_server.clear();
    LOG_DEBUG( "device server deleted @ '" << _topic_root << "'" );
}


void dds_device_server::start_streaming( const std::string & stream_name,
                                         const image_header & header )
{
    auto it = _stream_name_to_server.find( stream_name );
    if( it == _stream_name_to_server.end() )
        DDS_THROW( runtime_error, "stream '" + stream_name + "' does not exist" );
    auto& stream = it->second;
    stream->start_streaming( header );
}


void dds_device_server::publish_image( const std::string & stream_name, const uint8_t * data, size_t size )
{
    auto it = _stream_name_to_server.find( stream_name );
    if( it == _stream_name_to_server.end() )
        DDS_THROW( runtime_error, "stream '" + stream_name + "' does not exist" );
    auto & stream = it->second;
    if( ! stream->is_streaming() )
        DDS_THROW( runtime_error, "stream '" + stream_name + "' is not streaming" );

    stream->publish_image( data, size );
}


static void on_discovery_device_header( size_t const n_streams, dds_notification_server & notifications )
{
    topics::device::notification::device_header_msg device_header;
    device_header.num_of_streams = n_streams;

    topics::raw::device::notification notification;
    topics::device::notification::construct_raw_message( topics::device::notification::msg_type::DEVICE_HEADER,
                                                         device_header,
                                                         notification );
    notifications.add_discovery_notification( std::move( notification ) );
}


static void on_discovery_stream_header( std::shared_ptr< dds_stream_server > const & stream,
                                        dds_notification_server & notifications )
{
    // No stream header at this time
}


static void on_discovery_video_stream( std::shared_ptr< dds_video_stream_server > const & stream,
                                       dds_notification_server & notifications )
{
    // Send current stream header message
    on_discovery_stream_header( stream, notifications );

    // Send stream profiles
    topics::device::notification::video_stream_profiles_msg video_stream_profiles;
    video_stream_profiles.group_name[stream->name().copy( video_stream_profiles.group_name, sizeof( video_stream_profiles.group_name - 1 ))] = 0;
    int index = 0;
    for( auto & sp : stream->profiles() )
    {
        auto vsp = std::dynamic_pointer_cast< dds_video_stream_profile >( sp );
        if( ! vsp )
            DDS_THROW( runtime_error, "profile " + sp->to_string() + " is not a video profile" );
        topics::device::notification::video_stream_profile vsp_msg = { vsp->uid().index,
                                                                       vsp->uid().sid,
                                                                       vsp->frequency(),
                                                                       (int8_t)vsp->format().to_rs2(),
                                                                       0,  // RS2_STREAM_ANY,
                                                                       (int16_t)vsp->width(),
                                                                       (int16_t)vsp->height(),
                                                                       stream->default_profile_index() == index };

        video_stream_profiles.profiles[index++] = std::move( vsp_msg );
    }
    video_stream_profiles.num_of_profiles = index;

    topics::raw::device::notification notification;
    topics::device::notification::construct_raw_message( topics::device::notification::msg_type::VIDEO_STREAM_PROFILES,
                                                         video_stream_profiles,
                                                         notification );
    notifications.add_discovery_notification( std::move( notification ) );
}


static void on_discovery_motion_stream( std::shared_ptr< dds_motion_stream_server > const & stream,
                                        dds_notification_server & notifications )
{
    // Send current stream header message
    on_discovery_stream_header( stream, notifications );

    // Send stream profiles
    topics::device::notification::motion_stream_profiles_msg motion_stream_profiles;
    motion_stream_profiles.group_name[stream->name().copy( motion_stream_profiles.group_name, sizeof( motion_stream_profiles.group_name - 1 ))] = 0;
    int index = 0;
    for( auto & sp : stream->profiles() )
    {
        auto msp = std::dynamic_pointer_cast< dds_motion_stream_profile >( sp );
        if( ! msp )
            DDS_THROW( runtime_error, "profile " + sp->to_string() + " is not a motion profile" );
        topics::device::notification::motion_stream_profile msp_msg = { msp->uid().index,
                                                                        msp->uid().sid,
                                                                        msp->frequency(),
                                                                        (int8_t)msp->format().to_rs2(),
                                                                        0 };  // RS2_STREAM_ANY

        motion_stream_profiles.profiles[index++] = std::move( msp_msg );
    }
    motion_stream_profiles.num_of_profiles = index;

    topics::raw::device::notification notification;
    topics::device::notification::construct_raw_message( topics::device::notification::msg_type::MOTION_STREAM_PROFILES,
                                                         motion_stream_profiles,
                                                         notification );
    notifications.add_discovery_notification( std::move( notification ) );
}


void dds_device_server::init( std::vector< std::shared_ptr< dds_stream_server > > const & streams )
{
    if( is_valid() )
        DDS_THROW( runtime_error, "device server '" + _topic_root + "' is already initialized" );

    // Create a notifications server and set discovery notifications
    _notification_server
        = std::make_shared< dds_notification_server >( _publisher, _topic_root + "/notification" );

    on_discovery_device_header( streams.size(), *_notification_server );

    for( auto & stream : streams )
    {
        std::string topic_name = _topic_root + '/' + stream->name();
        stream->open( topic_name, _publisher );
        _stream_name_to_server[stream->name()] = stream;

        if( auto video_stream = std::dynamic_pointer_cast< dds_video_stream_server >( stream ) )
            on_discovery_video_stream( video_stream, *_notification_server );
        else if( auto motion_stream = std::dynamic_pointer_cast< dds_motion_stream_server >( stream ) )
            on_discovery_motion_stream( motion_stream, *_notification_server );
        else
            DDS_THROW( runtime_error, "unexpected stream '" + stream->name() + "' type" );
    }

    // Create a control server and set callback
    _control_server = std::make_shared< dds_control_server >( _subscriber, _topic_root + "/control" );

    _control_server->on_control_message_received( [&]() { on_control_message_received(); } );
}


void dds_device_server::publish_notification( topics::raw::device::notification&& notification )
{
    _notification_server->send_notification( std::move( notification ) );
}


void dds_device_server::on_control_message_received()
{
    //TODO - handle control
}

