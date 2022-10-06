// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-device-server.h>

#include <realdds/dds-participant.h>
#include <realdds/dds-publisher.h>
#include <realdds/dds-stream-server.h>
#include <realdds/dds-notification-server.h>
#include <realdds/dds-utilities.h>
#include <realdds/topics/dds-topics.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-topic-writer.h>

#include <fastdds/dds/topic/Topic.hpp>


using namespace eprosima::fastdds::dds;
using namespace realdds;


dds_device_server::dds_device_server( std::shared_ptr< dds_participant > const & participant,
                                      const std::string & topic_root )
    : _participant( participant )
    , _publisher( std::make_shared< dds_publisher >( participant ))
    , _topic_root( topic_root )
{
    LOG_DEBUG( "DDS device server for device topic root: " << _topic_root << " created" );
}


dds_device_server::~dds_device_server()
{
    _stream_name_to_server.clear();
    LOG_DEBUG( "DDS device server '" << _topic_root << "' deleted" );
}


void dds_device_server::start_streaming( const std::string & stream_name,
                                         const image_header & header )
{
    _stream_name_to_server.at( stream_name )->start_streaming( header );
}


void dds_device_server::publish_image( const std::string & stream_name, const uint8_t * data, size_t size )
{
    auto stream = _stream_name_to_server.at( stream_name );  // may throw!

    if( ! stream->is_streaming() )
        DDS_THROW( runtime_error,
                   "Cannot publish_image() to non-streaming '" << stream->get_topic()->get()->get_name() << "'" );

    stream->publish_image( data, size );
}


void dds_device_server::init( const std::vector<std::string> &supported_streams_names )
{
    if( is_valid() )
    {
        DDS_THROW( runtime_error, "device server '" << _topic_root << "' is already initialized" );
    }

    // Create a notifications server
    _dds_notification_server
        = std::make_shared< dds_notification_server >( _publisher, _topic_root + "/notification" );

    // Create streams servers per each supporting stream of the device
    for( auto stream_name : supported_streams_names )
    {
        std::string topic_name = _topic_root + '/' + stream_name;
        auto topic = topics::device::image::create_topic( _participant, topic_name.c_str() );
        
        // TODO:: Maybe we want to open a writer only when the stream is requested?
        auto writer = std::make_shared< dds_topic_writer >( topic, _publisher );
        writer->run( dds_topic_writer::writer_qos( BEST_EFFORT_RELIABILITY_QOS ));  // no retries

        _stream_name_to_server[stream_name] = std::make_shared< dds_stream_server >( writer );
    }
}


void dds_device_server::publish_notification( topics::raw::device::notification&& notification )
{
    _dds_notification_server->send_notification( std::move( notification ) );
}


void dds_device_server::add_init_msg( topics::raw::device::notification&& notification )
{
    _dds_notification_server->add_init_notification( std::move( notification ) );
}

