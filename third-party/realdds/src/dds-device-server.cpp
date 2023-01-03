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
#include <realdds/topics/dds-topic-names.h>
#include <realdds/topics/image/image-msg.h>
#include <realdds/topics/flexible/flexible-msg.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-topic-writer.h>
#include <realdds/dds-option.h>

#include <fastdds/dds/subscriber/SampleInfo.hpp>

#include <librealsense2/utilities/json.h>

using nlohmann::json;
using namespace eprosima::fastdds::dds;
using namespace realdds;


dds_device_server::dds_device_server( std::shared_ptr< dds_participant > const & participant,
                                      const std::string & topic_root )
    : _publisher( std::make_shared< dds_publisher >( participant ) )
    , _subscriber( std::make_shared< dds_subscriber >( participant ) )
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


void dds_device_server::stop_streaming( const std::vector< std::string > & stream_to_close )
{
    for ( auto & stream_name : stream_to_close )
    {
        auto it = _stream_name_to_server.find( stream_name );
        if ( it == _stream_name_to_server.end() )
            DDS_THROW( runtime_error, "stream '" + stream_name + "' does not exist" );
        auto & stream = it->second;
        stream->stop_streaming();
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


static void on_discovery_device_header( size_t const n_streams, const dds_options & options,
                                        dds_notification_server & notifications )
{
    topics::flexible_msg device_header( json{
        { "id", "device-header" },
        { "n-streams", n_streams },
    } );
    notifications.add_discovery_notification( std::move( device_header ) );

    auto device_options = nlohmann::json::array();
    for( auto & opt : options )
        device_options.push_back( std::move( opt->to_json() ) );
    topics::flexible_msg device_options_message( json {
        { "id", "device-options" },
        { "n-options", options.size() },
        { "options" , device_options }
        } );
    notifications.add_discovery_notification( std::move( device_options_message ) );
}


static void on_discovery_stream_header( std::shared_ptr< dds_stream_server > const & stream,
                                        dds_notification_server & notifications )
{
    auto profiles = nlohmann::json::array();
    for( auto & sp : stream->profiles() )
        profiles.push_back( std::move( sp->to_json() ) );
    topics::flexible_msg stream_header_message( json {
        { "id", "stream-header" },
        { "type", stream->type_string() },
        { "name", stream->name() },
        { "sensor-name", stream->sensor_name() },
        { "profiles", profiles },
        { "default-profile-index", stream->default_profile_index() },
    } );
    LOG_DEBUG( "-----> JSON = " << stream_header_message.json_data().dump() );
    LOG_DEBUG( "-----> JSON size = " << stream_header_message.json_data().dump().length() );
    LOG_DEBUG( "-----> CBOR size = " << json::to_cbor( stream_header_message.json_data() ).size() );
    notifications.add_discovery_notification( std::move( stream_header_message ) );

    auto stream_options = nlohmann::json::array();
    for( auto & opt : stream->options() )
        stream_options.push_back( std::move( opt->to_json() ) );
    auto video_stream = std::dynamic_pointer_cast< dds_video_stream_server >( stream );
    auto motion_server = std::dynamic_pointer_cast< dds_motion_stream_server >( stream );
    auto intrinsics = nlohmann::json::array();
    if( video_stream )
        for( auto & intr : video_stream->get_intrinsics() )
            intrinsics.push_back( intr.to_json() );
    if( motion_server )
        intrinsics.push_back( motion_server->get_intrinsics().to_json() );
    topics::flexible_msg stream_options_message( json {
        { "id", "stream-options" },
        { "stream-name", stream->name() },
        { "n-options", stream->options().size() },
        { "options" , stream_options },
        { "intrinsics" , intrinsics }
    } );
    LOG_DEBUG( "-----> JSON = " << stream_options_message.json_data().dump() );
    LOG_DEBUG( "-----> JSON size = " << stream_options_message.json_data().dump().length() );
    LOG_DEBUG( "-----> CBOR size = " << json::to_cbor( stream_options_message.json_data() ).size() );
    notifications.add_discovery_notification( std::move( stream_options_message ) );
}


void dds_device_server::init( std::vector< std::shared_ptr< dds_stream_server > > const & streams,
                              const dds_options & options)
{
    if( is_valid() )
        DDS_THROW( runtime_error, "device server '" + _topic_root + "' is already initialized" );

    try
    {
        // Create a notifications server and set discovery notifications
        _notification_server = std::make_shared< dds_notification_server >( _publisher, _topic_root + topics::NOTIFICATION_TOPIC_NAME );

        // If a previous init failed (e.g., one of the streams has no profiles):
        _stream_name_to_server.clear();

        on_discovery_device_header( streams.size(), options, *_notification_server );
        for( auto& stream : streams )
        {
            std::string topic_name = _topic_root + '/' + stream->name();
            stream->open( topic_name, _publisher );
            _stream_name_to_server[stream->name()] = stream;
            on_discovery_stream_header( stream, *_notification_server );
        }

        _notification_server->run();

        // Create a control reader and set callback
        auto topic = topics::flexible_msg::create_topic( _subscriber->get_participant(), _topic_root + topics::CONTROL_TOPIC_NAME );
        _control_reader = std::make_shared< dds_topic_reader >( topic, _subscriber );

        _control_reader->on_data_available( [&]() { on_control_message_received(); } );

        dds_topic_reader::qos rqos( RELIABLE_RELIABILITY_QOS );
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

        _control_dispatcher.invoke( [x = std::move(data), this]( dispatcher::cancellable_timer ) { handle_control_message( x ); } );
    }
}

void dds_device_server::handle_control_message( topics::flexible_msg control_message )
{
    auto j = control_message.json_data();
    auto id = utilities::json::get< std::string >( j, "id" );
    LOG_DEBUG( "-----> JSON = " << j.dump() );

    if ( id.compare("open-streams") == 0 )
    {
        if ( _open_streams_callback )
            _open_streams_callback( j );
    }
    else if( id.compare( "close-streams" ) == 0 )
    {
        if ( _close_streams_callback )
            _close_streams_callback( j );
    }
    else if( id.compare( "set-option" ) == 0 )
    {
        handle_set_option( j );
    }
    else if( id.compare( "query-option" ) == 0 )
    {
        handle_query_option( j );
    }
}

void dds_device_server::handle_set_option( const nlohmann::json & j )
{
    const std::string & option_name = utilities::json::get< std::string >( j, "option-name" );
    const std::string & owner_name = utilities::json::get< std::string >( j, "owner-name" );
    uint32_t counter = utilities::json::get< uint32_t >( j, "counter" );

    std::shared_ptr< dds_option > opt = find_option( option_name, owner_name );

    if( opt )
    {
        try
        {
            float value = utilities::json::get< float >( j, "value" );
            _set_option_callback( opt, value ); //Handle setting option outside realdds
            opt->set_value( value ); //Update option object. Do second to check if _set_option_callback did not throw
            send_set_option_success( counter );
        }
        catch( std::exception e )
        {
            send_set_option_failure( counter, e.what() );
        }
    }
    else
    {
        send_set_option_failure( counter, owner_name + " does not support option " + option_name );
    }
}

void dds_device_server::handle_query_option( const nlohmann::json & j )
{
    const std::string & option_name = utilities::json::get< std::string >( j, "option-name" );
    const std::string & owner_name = utilities::json::get< std::string >( j, "owner-name" );
    uint32_t counter = utilities::json::get< uint32_t >( j, "counter" );

    std::shared_ptr< dds_option > opt = find_option( option_name, owner_name );

    if( opt )
    {
        try
        {
            float value = _query_option_callback( opt ); //Handle query outside realdds
            opt->set_value( value ); //Ensure realdds option is up to date with actual value (might change, e.g temperature)
            send_query_option_success( counter, value );
        }
        catch( std::exception e )
        {
            send_query_option_failure( counter, e.what() );
        }
    }
    else
    {
        send_query_option_failure( counter, owner_name + " does not support option " + option_name );
    }
}

std::shared_ptr< dds_option > realdds::dds_device_server::find_option( const std::string & option_name,
                                                                       const std::string & owner_name )
{
    if( _topic_root == owner_name )
    {
        //TODO - handle device option
    }
    else
    {
        //Find option in owner stream
        for( auto & stream_it : _stream_name_to_server )
        {
            if( stream_it.first == owner_name )
            {
                for( auto & option : stream_it.second->options() )
                {
                    if( option->get_name() == option_name )
                    {
                        return option;
                    }
                }
            }
        }
    }

    return nullptr;
}

void realdds::dds_device_server::send_set_option_success( uint32_t counter )
{
    topics::flexible_msg notification( json {
        { "id", "set-option" },
        { "counter", counter },
        { "successful", true },
        { "failure-reason", "" },
    } );
    publish_notification( std::move( notification ) );
}

void realdds::dds_device_server::send_set_option_failure( uint32_t counter, const std::string & fail_reason )
{
    topics::flexible_msg notification( json {
        { "id", "set-option" },
        { "counter", counter },
        { "successful", false },
        { "failure-reason", fail_reason },
    } );
    publish_notification( std::move( notification ) );
}

void dds_device_server::send_query_option_success( uint32_t counter, float value )
{
    topics::flexible_msg notification( json {
        { "id", "query-option" },
        { "counter", counter },
        { "value", value },
        { "successful", true },
        { "failure-reason", "" },
    } );

    publish_notification( std::move( notification ) );
}

void realdds::dds_device_server::send_query_option_failure( uint32_t counter, const std::string & fail_reason )
{
    topics::flexible_msg notification( json {
        { "id", "query-option" },
        { "counter", counter },
        { "value", 0.0f },
        { "successful", false },
        { "failure-reason", fail_reason },
    } );
    publish_notification( std::move( notification ) );
}
