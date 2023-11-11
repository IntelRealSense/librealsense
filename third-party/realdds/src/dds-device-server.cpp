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
#include <realdds/dds-device-broadcaster.h>
#include <realdds/dds-utilities.h>
#include <realdds/topics/dds-topic-names.h>
#include <realdds/topics/device-info-msg.h>
#include <realdds/topics/flexible-msg.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-topic-writer.h>
#include <realdds/dds-option.h>
#include <realdds/dds-guid.h>

#include <fastdds/dds/subscriber/SampleInfo.hpp>

#include <rsutils/string/shorten-json-string.h>
#include <rsutils/json.h>
using nlohmann::json;
using rsutils::string::slice;
using rsutils::string::shorten_json_string;

using namespace eprosima::fastdds::dds;
using namespace realdds;


static std::string const id_key( "id", 2 );
static std::string const id_set_option( "set-option", 10 );
static std::string const id_query_option( "query-option", 12 );
static std::string const value_key( "value", 5 );
static std::string const sample_key( "sample", 6 );
static std::string const status_key( "status", 6 );
static std::string const status_ok( "ok", 2 );
static std::string const option_name_key( "option-name", 11 );
static std::string const stream_name_key( "stream-name", 11 );
static std::string const explanation_key( "explanation", 11 );
static std::string const control_key( "control", 7 );


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


dds_guid const & dds_device_server::guid() const
{
    return _notification_server ? _notification_server->guid() : unknown_guid;
}



dds_device_server::~dds_device_server()
{
    _stream_name_to_server.clear();
    LOG_DEBUG( "device server deleted @ '" << _topic_root << "'" );
}


static void on_discovery_device_header( size_t const n_streams,
                                        const dds_options & options,
                                        const extrinsics_map & extr,
                                        dds_notification_server & notifications )
{
    auto extrinsics_json = json::array();
    for( auto & ex : extr )
        extrinsics_json.push_back( json::array( { ex.first.first, ex.first.second, ex.second->to_json() } ) );

    topics::flexible_msg device_header( json{
        { id_key, "device-header" },
        { "n-streams", n_streams },
        { "extrinsics", std::move( extrinsics_json ) }
    } );
    auto json_string = slice( device_header.custom_data< char const >(), device_header._data.size() );
    LOG_DEBUG( "-----> JSON = " << shorten_json_string( json_string, 300 ) << " size " << json_string.length() );
    //LOG_DEBUG( "-----> CBOR size = " << json::to_cbor( device_header.json_data() ).size() );
    notifications.add_discovery_notification( std::move( device_header ) );

    auto device_options = nlohmann::json::array();
    for( auto & opt : options )
        device_options.push_back( std::move( opt->to_json() ) );
    topics::flexible_msg device_options_message( json {
        { id_key, "device-options" },
        { "options", std::move( device_options ) }
    } );
    json_string = slice( device_options_message.custom_data< char const >(), device_options_message._data.size() );
    LOG_DEBUG( "-----> JSON = " << shorten_json_string( json_string, 300 ) << " size " << json_string.length() );
    //LOG_DEBUG( "-----> CBOR size = " << json::to_cbor( device_options_message.json_data() ).size() );
    notifications.add_discovery_notification( std::move( device_options_message ) );
}


static void on_discovery_stream_header( std::shared_ptr< dds_stream_server > const & stream,
                                        dds_notification_server & notifications )
{
    auto profiles = nlohmann::json::array();
    for( auto & sp : stream->profiles() )
        profiles.push_back( std::move( sp->to_json() ) );
    topics::flexible_msg stream_header_message( json{
        { id_key, "stream-header" },
        { "type", stream->type_string() },
        { "name", stream->name() },
        { "sensor-name", stream->sensor_name() },
        { "profiles", std::move( profiles ) },
        { "default-profile-index", stream->default_profile_index() },
        { "metadata-enabled", stream->metadata_enabled() },
    } );
    auto json_string = slice( stream_header_message.custom_data< char const >(), stream_header_message._data.size() );
    LOG_DEBUG( "-----> JSON = " << shorten_json_string( json_string, 300 ) << " size " << json_string.length() );
    //LOG_DEBUG( "-----> CBOR size = " << json::to_cbor( stream_header_message.json_data() ).size() );
    notifications.add_discovery_notification( std::move( stream_header_message ) );

    auto stream_options = nlohmann::json::array();
    for( auto & opt : stream->options() )
        stream_options.push_back( std::move( opt->to_json() ) );

    nlohmann::json intrinsics;
    if( auto video_stream = std::dynamic_pointer_cast< dds_video_stream_server >( stream ) )
    {
        intrinsics = nlohmann::json::array();
        for( auto & intr : video_stream->get_intrinsics() )
            intrinsics.push_back( intr.to_json() );
    }
    else if( auto motion_stream = std::dynamic_pointer_cast< dds_motion_stream_server >( stream ) )
    {
        intrinsics = nlohmann::json::object( {
            { "accel", motion_stream->get_accel_intrinsics().to_json() },
            { "gyro", motion_stream->get_gyro_intrinsics().to_json() }
        } );
    }

    auto stream_filters = nlohmann::json::array();
    for( auto & filter : stream->recommended_filters() )
        stream_filters.push_back( filter );
    topics::flexible_msg stream_options_message( json {
        { id_key, "stream-options" },
        { "stream-name", stream->name() },
        { "options" , std::move( stream_options ) },
        { "intrinsics" , intrinsics },
        { "recommended-filters", std::move( stream_filters ) },
    } );
    json_string = slice( stream_options_message.custom_data< char const >(), stream_options_message._data.size() );
    LOG_DEBUG( "-----> JSON = " << shorten_json_string( json_string, 300 ) << " size " << json_string.length() );
    //LOG_DEBUG( "-----> CBOR size = " << json::to_cbor( stream_options_message.json_data() ).size() );
    notifications.add_discovery_notification( std::move( stream_options_message ) );
}


static std::string ros_friendly_topic_name( std::string name )
{
    // Replace all '/' with '_'
    int n = 0;
    for( char & ch : name )
        if( ch == '/' )
            if( ++n > 1 )
                ch = '_';
    // ROS topics start with "rt/" prefix
    name.insert( 0, "rt/", 3 );
    return name;
}


void dds_device_server::init( std::vector< std::shared_ptr< dds_stream_server > > const & streams,
                              const dds_options & options, const extrinsics_map & extr )
{
    if( is_valid() )
        DDS_THROW( runtime_error, "device server '" + _topic_root + "' is already initialized" );

    try
    {
        // Create a notifications server and set discovery notifications
        _notification_server = std::make_shared< dds_notification_server >( _publisher,
                                                                            _topic_root + topics::NOTIFICATION_TOPIC_NAME );

        // If a previous init failed (e.g., one of the streams has no profiles):
        _stream_name_to_server.clear();

        _options = options;
        on_discovery_device_header( streams.size(), options, extr, *_notification_server );
        for( auto & stream : streams )
        {
            std::string topic_name = ros_friendly_topic_name( _topic_root + '/' + stream->name() );
            stream->open( topic_name, _publisher );
            _stream_name_to_server[stream->name()] = stream;
            on_discovery_stream_header( stream, *_notification_server );

            if( stream->metadata_enabled() && ! _metadata_writer )
            {
                auto topic = topics::flexible_msg::create_topic( _publisher->get_participant(),
                                                                 _topic_root + topics::METADATA_TOPIC_NAME );
                _metadata_writer = std::make_shared< dds_topic_writer >( topic, _publisher );
                dds_topic_writer::qos wqos( eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS );
                wqos.history().depth = 10;  // default is 1
                wqos.override_from_json(
                    rsutils::json::nested( _subscriber->get_participant()->settings(), "device", "metadata" ) );
                _metadata_writer->run( wqos );
            }
        }

        _notification_server->run();

        // Create a control reader and set callback
        auto topic = topics::flexible_msg::create_topic( _subscriber->get_participant(), _topic_root + topics::CONTROL_TOPIC_NAME );
        _control_reader = std::make_shared< dds_topic_reader >( topic, _subscriber );

        _control_reader->on_data_available( [&]() { on_control_message_received(); } );

        dds_topic_reader::qos rqos( RELIABLE_RELIABILITY_QOS );
        rqos.override_from_json( rsutils::json::nested( _subscriber->get_participant()->settings(), "device", "control" ) );
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


void dds_device_server::broadcast( topics::device_info const & device_info )
{
    if( _broadcaster )
        DDS_THROW( runtime_error, "device server was already broadcast" );
    if( device_info.topic_root != _topic_root )
        DDS_THROW( runtime_error, "topic roots do not match" );
    _broadcaster = std::make_shared< dds_device_broadcaster >( _publisher, device_info );
}


void dds_device_server::publish_notification( topics::flexible_msg && notification )
{
    _notification_server->send_notification( std::move( notification ) );
}


void dds_device_server::publish_metadata( nlohmann::json && md )
{
    if( ! _metadata_writer )
        DDS_THROW( runtime_error, "device '" + _topic_root + "' has no stream with enabled metadata" );

    topics::flexible_msg msg( md );
    LOG_DEBUG(
        "publishing metadata: " << shorten_json_string( slice( msg.custom_data< char const >(), msg._data.size() ),
                                                        300 ) );
    std::move( msg ).write_to( *_metadata_writer );
}


bool dds_device_server::has_metadata_readers() const
{
    return _metadata_writer && _metadata_writer->has_readers();
}


void dds_device_server::on_control_message_received()
{
    topics::flexible_msg data;
    eprosima::fastdds::dds::SampleInfo info;
    while( topics::flexible_msg::take_next( *_control_reader, &data, &info ) )
    {
        if( ! data.is_valid() )
            continue;

        _control_dispatcher.invoke(
            [j = data.json_data(), sample = info, this]( dispatcher::cancellable_timer )
            {
                json reply;
                reply[sample_key] = json::array( {
                    realdds::print( sample.sample_identity.writer_guid(), false ),  // raw guid
                    sample.sample_identity.sequence_number().to64long(),
                } );
                try
                {
                    std::string id = rsutils::json::get< std::string >( j, id_key );
                    reply[id_key] = id;
                    reply[control_key] = j;
                    handle_control_message( id, j, reply );
                }
                catch( std::exception const & e )
                {
                    reply[status_key] = "error";
                    reply[explanation_key] = e.what();
                }
                try
                {
                    LOG_DEBUG( "----->   reply " << reply );
                    publish_notification( reply );
                }
                catch( ... )
                {
                    LOG_ERROR( "failed to send reply" );
                }
            } );
    }
}


void dds_device_server::handle_control_message( std::string const & id,
                                                nlohmann::json const & j,
                                                nlohmann::json & reply )
{
    LOG_DEBUG( "<----- control " << j );

    if( id.compare( id_set_option ) == 0 )
    {
        handle_set_option( j, reply );
    }
    else if( id.compare( id_query_option ) == 0 )
    {
        handle_query_option( j, reply );
    }
    else if( ! _control_callback || ! _control_callback( id, j, reply ) )
    {
        DDS_THROW( runtime_error, "invalid control '" + id + "'" );
    }
}


void dds_device_server::handle_set_option( const nlohmann::json & j, nlohmann::json & reply )
{
    auto option_name = rsutils::json::get< std::string >( j, option_name_key );
    std::string stream_name;  // default is empty, for a device option
    rsutils::json::get_ex( j, stream_name_key, &stream_name );

    std::shared_ptr< dds_option > opt = find_option( option_name, stream_name );
    if( opt )
    {
        float value = rsutils::json::get< float >( j, value_key );
        if( _set_option_callback )
            _set_option_callback( opt, value ); //Handle setting option outside realdds
        opt->set_value( value ); //Update option object. Do second to check if _set_option_callback did not throw
        reply[value_key] = value;
    }
    else
    {
        if( stream_name.empty() )
            stream_name = "device";
        else
            stream_name = "'" + stream_name + "'";
        DDS_THROW( runtime_error, stream_name + " option '" + option_name + "' not found" );
    }
}


void dds_device_server::handle_query_option( const nlohmann::json & j, nlohmann::json & reply )
{
    auto option_name = rsutils::json::get< std::string >( j, option_name_key );
    std::string stream_name;  // default is empty, for a device option
    rsutils::json::get_ex( j, stream_name_key, &stream_name );

    std::shared_ptr< dds_option > opt = find_option( option_name, stream_name );
    if( opt )
    {
        float value;
        if( _query_option_callback )
        {
            value = _query_option_callback( opt );
            // Ensure realdds option is up to date with actual value from callback
            opt->set_value( value );
        }
        else
        {
            value = opt->get_value();
        }
        reply[value_key] = value;
    }
    else
    {
        if( stream_name.empty() )
            stream_name = "device";
        else
            stream_name = "'" + stream_name + "'";
        DDS_THROW( runtime_error, stream_name + " option '" + option_name + "' not found" );
    }
}


std::shared_ptr< dds_option > realdds::dds_device_server::find_option( const std::string & option_name,
                                                                       const std::string & stream_name ) const
{
    if( stream_name.empty() )
    {
        for( auto & option : _options )
        {
            if( option->get_name() == option_name )
                return option;
        }
    }
    else
    {
        // Find option in owner stream
        auto stream_it = _stream_name_to_server.find( stream_name );
        if( stream_it != _stream_name_to_server.end() )
        {
            for( auto & option : stream_it->second->options() )
            {
                if( option->get_name() == option_name )
                    return option;
            }
        }
    }

    return {};
}
