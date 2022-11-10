// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-device-server.h>

#include <realdds/dds-participant.h>
#include <realdds/dds-publisher.h>
#include <realdds/dds-subscriber.h>
#include <realdds/dds-stream-server.h>
#include <realdds/dds-stream-profile.h>
#include <realdds/dds-notification-server.h>
#include <realdds/dds-topic-reader.h>
#include <realdds/dds-utilities.h>
#include <realdds/topics/device-info/device-info-msg.h>
#include <realdds/topics/image/image-msg.h>
#include <realdds/topics/flexible/flexible-msg.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-topic-writer.h>

#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>

using nlohmann::json;
using namespace eprosima::fastdds::dds;
using namespace realdds;


dds_device_server::dds_device_server( std::shared_ptr< dds_participant > const & participant,
                                      const std::string & topic_root )
    : _publisher( std::make_shared< dds_publisher >( participant ))
    , _subscriber( std::make_shared< dds_subscriber >( participant ))
    , _topic_root( topic_root )
    , _control_dispatcher( QUEUE_MAX_SIZE )
{
    LOG_DEBUG( "device server created @ '" << _topic_root << "'" );
    _control_dispatcher.start();
}


dds_device_server::~dds_device_server()
{
    _stream_name_to_server.clear();
    LOG_DEBUG( "device server deleted @ '" << _topic_root << "'" );
}


void dds_device_server::start_streaming( const std::vector< std::pair < std::string, image_header > > & streams_to_open )
{
    for ( auto & p : streams_to_open )
    {
        auto & stream_name = p.first;
        auto & header = p.second;
        auto it = _stream_name_to_server.find( stream_name );
        if ( it == _stream_name_to_server.end() )
            DDS_THROW( runtime_error, "stream '" + stream_name + "' does not exist" );
        auto & stream = it->second;
        stream->start_streaming( header );
    }
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
    topics::flexible_msg notification( json{
        { "id", "device-header" },
        { "n-streams", n_streams },
    } );
    notifications.add_discovery_notification( std::move( notification ) );
}


static void on_discovery_stream_header( std::shared_ptr< dds_stream_server > const & stream,
                                        dds_notification_server & notifications )
{
    auto profiles = nlohmann::json::array();
    for( auto & sp : stream->profiles() )
        profiles.push_back( std::move( sp->to_json() ) );
    json msg = {
        { "id", "stream-header" },
        { "type", stream->type_string() },
        { "name", stream->name() },
        { "sensor-name", stream->sensor_name() },
        { "profiles", profiles },
        { "default-profile-index", stream->default_profile_index() },
    };
    LOG_DEBUG( "-----> JSON = " << msg.dump() );
    LOG_DEBUG( "-----> JSON size = " << msg.dump().length() );
    LOG_DEBUG( "-----> CBOR size = " << json::to_cbor( msg ).size() );

    topics::flexible_msg notification( msg );
    notifications.add_discovery_notification( std::move( notification ) );
}


void dds_device_server::init( std::vector< std::shared_ptr< dds_stream_server > > const & streams )
{
    if( is_valid() )
        DDS_THROW( runtime_error, "device server '" + _topic_root + "' is already initialized" );

    try
    {
        // Create a notifications server and set discovery notifications
        _notification_server = std::make_shared< dds_notification_server >( _publisher, _topic_root + "/notification" );

        // If a previous init failed (e.g., one of the streams has no profiles):
        _stream_name_to_server.clear();

        on_discovery_device_header( streams.size(), *_notification_server );
        for( auto& stream : streams )
        {
            std::string topic_name = _topic_root + '/' + stream->name();
            stream->open( topic_name, _publisher );
            _stream_name_to_server[stream->name()] = stream;
            on_discovery_stream_header( stream, *_notification_server );
        }

        _notification_server->run();

        // Create a control reader and set callback
        auto topic = topics::flexible_msg::create_topic( _subscriber->get_participant(), _topic_root + "/control" );
        _control_reader = std::make_shared< dds_topic_reader >( topic, _subscriber );

        dds_topic_reader::qos rqos( RELIABLE_RELIABILITY_QOS );
        rqos.history().depth = 10; // default is 1
        // Does not allocate for every sample but still gives flexibility. See:
        //     https://github.com/eProsima/Fast-DDS/discussions/2707
        // (default is PREALLOCATED_MEMORY_MODE)
        rqos.endpoint().history_memory_policy = eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;

        //_control_server->on_control_message_received( [&]() { on_control_message_received(); } );
        _control_reader->on_data_available( [&]() { on_control_message_received(); } );
        _control_reader->run( rqos );
    }
    catch( std::exception const & )
    {
        _notification_server.reset();
        _stream_name_to_server.clear();
        _control_reader.reset();
        throw;
    }
}


void dds_device_server::publish_notification( topics::flexible_msg && notification )
{
    _notification_server->send_notification( std::move( notification ) );
}


void dds_device_server::on_control_message_received()
{
    topics::flexible_msg data;
    eprosima::fastdds::dds::SampleInfo info;
    while ( topics::flexible_msg::take_next( *_control_reader, &data, &info ) )
    {
        if ( !data.is_valid() )
            continue;

        _control_dispatcher.invoke( [&]( dispatcher::cancellable_timer ) { handle_control_message( data ); } );
    }
}

void dds_device_server::handle_control_message( topics::flexible_msg control_message )
{
    auto const & j = control_message.json_data();
    auto id = j["id"].get< std::string >();
    if ( id.compare("open-streams") == 0 )
    {
        if ( _open_streams_callback )
            _open_streams_callback( j, this );
    }
}
