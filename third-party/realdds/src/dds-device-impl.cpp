// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "dds-device-impl.h"

#include <realdds/dds-participant.h>
#include <realdds/dds-topic-reader-thread.h>
#include <realdds/dds-topic-writer.h>
#include <realdds/dds-subscriber.h>
#include <realdds/dds-option.h>
#include <realdds/topics/dds-topic-names.h>
#include <realdds/topics/flexible-msg.h>
#include <realdds/dds-guid.h>
#include <realdds/dds-time.h>

#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>

#include <rsutils/time/timer.h>
#include <rsutils/string/from.h>
#include <rsutils/json.h>

#include <cassert>


static std::string const id_key( "id", 2 );
static std::string const id_set_option( "set-option", 10 );
static std::string const id_query_option( "query-option", 12 );
static std::string const id_open_streams( "open-streams", 12 );
static std::string const id_device_header( "device-header", 13 );
static std::string const id_device_options( "device-options", 14 );
static std::string const id_stream_header( "stream-header", 13 );
static std::string const id_stream_options( "stream-options", 14 );
static std::string const value_key( "value", 5 );
static std::string const sample_key( "sample", 6 );
static std::string const status_key( "status", 6 );
static std::string const status_ok( "ok", 2 );
static std::string const option_name_key( "option-name", 11 );
static std::string const stream_name_key( "stream-name", 11 );
static std::string const explanation_key( "explanation", 11 );
static std::string const control_key( "control", 7 );

static std::string const id_log( "log", 3 );
static std::string const entries_key( "entries", 7 );

static std::string const id_hwm( "hwm", 3 );


namespace {


nlohmann::json device_settings( std::shared_ptr< realdds::dds_participant > const & participant )
{
    nlohmann::json settings = rsutils::json::nested( participant->settings(), "device" );
    if( settings.is_null() )
        // Nothing there: default is empty object
        return nlohmann::json::object();
    if( ! settings.is_object() )
        // Device settings, if they exist, must be an object!
        DDS_THROW( runtime_error, "participant 'device' settings must be an object: " << settings );
    return settings;
}


}


namespace realdds {


/*static*/ char const * dds_device::impl::to_string( state_t st )
{
    switch( st )
    {
    case state_t::WAIT_FOR_DEVICE_HEADER:
        return "WAIT_FOR_DEVICE_HEADER";
    case state_t::WAIT_FOR_DEVICE_OPTIONS:
        return "WAIT_FOR_DEVICE_OPTIONS";
    case state_t::WAIT_FOR_STREAM_HEADER:
        return "WAIT_FOR_STREAM_HEADER";
    case state_t::WAIT_FOR_STREAM_OPTIONS:
        return "WAIT_FOR_STREAM_OPTIONS";
    case state_t::READY:
        return "READY";
    default:
        return "UNKNOWN";
    }
}


void dds_device::impl::set_state( state_t new_state )
{
    if( new_state == _state )
        return;

    if( state_t::READY == new_state )
    {
        if( _metadata_reader )
        {
            nlohmann::json md_settings = rsutils::json::nested( _device_settings, "metadata" );
            if( ! md_settings.is_null() && ! md_settings.is_object() )  // not found is null
            {
                LOG_DEBUG( "... metadata is available but device/metadata is disabled" );
                _metadata_reader.reset();
            }
            else
            {
                LOG_DEBUG( "... metadata is enabled" );
                dds_topic_reader::qos rqos( eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS );
                rqos.history().depth = 10; // Support receive metadata from multiple streams
                rqos.override_from_json( md_settings );
                _metadata_reader->run( rqos );
            }
        }
        LOG_DEBUG( "device '" << _info.debug_name() << "' (" << _participant->print( guid() ) << ") is ready" );
    }

    _state = new_state;
}


/*static*/ dds_device::impl::notification_handlers const dds_device::impl::_notification_handlers{
    { id_set_option, &dds_device::impl::on_option_value },
    { id_query_option, &dds_device::impl::on_option_value },
    { id_device_header, &dds_device::impl::on_device_header },
    { id_device_options, &dds_device::impl::on_device_options },
    { id_stream_header, &dds_device::impl::on_stream_header },
    { id_stream_options, &dds_device::impl::on_stream_options },
    { id_open_streams, &dds_device::impl::on_known_notification },
    { id_log, &dds_device::impl::on_log },
    { id_hwm, &dds_device::impl::on_known_notification },
};


dds_device::impl::impl( std::shared_ptr< dds_participant > const & participant,
                        topics::device_info const & info )
    : _info( info )
    , _participant( participant )
    , _subscriber( std::make_shared< dds_subscriber >( participant ) )
    , _device_settings( device_settings( participant ) )
    , _reply_timeout_ms(
          rsutils::json::nested( _device_settings, "control", "reply-timeout-ms" ).value< size_t >( 2000 ) )
{
    create_notifications_reader();
    create_control_writer();
}


dds_guid const & dds_device::impl::guid() const
{
    return _control_writer->guid();
}


void dds_device::impl::wait_until_ready( size_t timeout_ms )
{
    if( is_ready() )
        return;

    LOG_DEBUG( "waiting for '" << _info.debug_name() << "' ..." );
    rsutils::time::timer timer{ std::chrono::milliseconds( timeout_ms ) };
    do
    {
        if( timer.has_expired() )
            DDS_THROW( runtime_error, "timeout waiting for '" << _info.debug_name() << "'" );
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    }
    while( ! is_ready() );
}


void dds_device::impl::handle_notification( nlohmann::json const & j,
                                            eprosima::fastdds::dds::SampleInfo const & sample )
{
    try
    {
        // First handle the notification
        auto id = rsutils::json::get< std::string >( j, id_key );
        auto it = _notification_handlers.find( id );
        if( it != _notification_handlers.end() )
            ( this->*( it->second ) )( j, sample );
        else if( ! _on_notification || ! _on_notification( id, j ) )
            throw std::runtime_error( "unhandled" );
    }
    catch( std::exception const & e )
    {
        LOG_DEBUG( "notification error: " << e.what() << "  " << j );
    }
    catch( ... )
    {
        LOG_DEBUG( "notification error: unknown exception  " << j );
    }

    try
    {
        // Check if this is a reply - maybe someone's waiting on it...
        auto sampleit = j.find( sample_key );
        if( sampleit != j.end() )
        {
            nlohmann::json const & sample = *sampleit;  // ["<prefix>.<entity>", <sequence-number>]
            if( sample.size() == 2 && sample.is_array() )
            {
                // We have to be the ones who sent the control!
                auto const guid_string = rsutils::json::get< std::string >( sample, 0 );
                auto const control_guid_string = realdds::print( _control_writer->get()->guid(), false );  // raw guid
                if( guid_string == control_guid_string )
                {
                    auto const sequence_number = rsutils::json::get< uint64_t >( sample, 1 );
                    std::unique_lock< std::mutex > lock( _replies_mutex );
                    auto replyit = _replies.find( sequence_number );
                    if( replyit != _replies.end() )
                    {
                        replyit->second = std::move( j );
                        _replies_cv.notify_all();
                    }
                }
            }
        }
    }
    catch( std::exception const & e )
    {
        LOG_DEBUG( "reply error: " << e.what() << "  " << j );
    }
    catch( ... )
    {
        LOG_DEBUG( "reply error: unknown exception  " << j );
    }
}


void dds_device::impl::on_option_value( nlohmann::json const & j, eprosima::fastdds::dds::SampleInfo const & )
{
    if( ! is_ready() )
        return;

    // This is the notification for "set-option" or "query-option", meaning someone sent a control request to set/get an
    // option value. In either case a value will be sent; we want to update ours accordingly to reflect the latest:
    if( rsutils::json::get( j, status_key, status_ok ) != status_ok )
    {
        // Ignore errors
        throw std::runtime_error( "status not OK" );
    }
    float new_value;
    if( ! rsutils::json::get_ex( j, value_key, &new_value ) )
    {
        throw std::runtime_error( "missing value" );
    }
    // We need the original control request as part of the reply, otherwise we can't know what option this is for
    auto it = j.find( control_key );
    if( it == j.end() )
    {
        throw std::runtime_error( "missing control" );
    }
    nlohmann::json const & control = it.value();
    if( ! control.is_object() )
    {
        throw std::runtime_error( "control is not an object" );
    }
    std::string option_name;
    if( ! rsutils::json::get_ex( control, option_name_key, &option_name ) )
    {
        throw std::runtime_error( "missing control/option-name" );
    }
    
    // Find the option and set its value
    dds_options const * options = &_options;
    std::string stream_name;  // default = empty = device option
    if( rsutils::json::get_ex( control, stream_name_key, &stream_name ) && !stream_name.empty() )
    {
        auto stream_it = _streams.find( stream_name );
        if( stream_it == _streams.end() )
        {
            throw std::runtime_error( "owner not found" );
        }
        options = &stream_it->second->options();
    }
    for( auto & option : *options )
    {
        if( option->get_name() == option_name )
        {
            option->set_value( new_value );
            return;
        }
    }
    throw std::runtime_error( "option not found" );
}


void dds_device::impl::on_known_notification( nlohmann::json const & j, eprosima::fastdds::dds::SampleInfo const & )
{
    // This is a known notification, but we don't want to do anything for it
}


void dds_device::impl::on_log( nlohmann::json const & j, eprosima::fastdds::dds::SampleInfo const & )
{
    // This is the notification for "log"  (see docs/notifications.md#Logging)
    //     - `entries` is an array containing 1 or more log entries
    auto it = j.find( entries_key );
    if( it == j.end() )
        throw std::runtime_error( "log entries not found" );
    if( ! it->is_array() )
        throw std::runtime_error( "log entries not an array" );
    // Each log entry is a JSON array of `[timestamp, type, text, data]` containing:
    //     - `timestamp`: when the event occurred
    //     - `type`: one of `EWID` (Error, Warning, Info, Debug)
    //     - `text`: any text that needs output
    //     - `data`: optional; an object containing any pertinent information about the event
    size_t x = 0;
    for( auto & entry : *it )
    {
        try
        {
            if( ! entry.is_array() )
                throw std::runtime_error( "not an array" );
            if( entry.size() > 4 )
                throw std::runtime_error( "too long" );
            auto timestamp = time_from( rsutils::json::get< dds_nsec >( entry, 0 ) );
            auto const stype = rsutils::json::get< std::string >( entry, 1 );
            if( stype.length() != 1 || ! strchr( "EWID", stype[0] ) )
                throw std::runtime_error( "type not one of 'EWID'" );
            char const type = stype[0];
            auto const text = rsutils::json::get< std::string >( entry, 2 );
            nlohmann::json data;
            if( entry.size() > 3 )
            {
                data = entry.at( 3 );
                if( ! data.is_object() )
                    throw std::runtime_error( "data is not an object" );
            }

            if( _on_device_log )
                _on_device_log( timestamp, type, text, data );
            else
                LOG_DEBUG( "[" << _info.debug_name() << "][" << timestr( timestamp ) << "][" << type << "] " << text
                               << " [" << data << "]" );
        }
        catch( std::exception const & e )
        {
            LOG_DEBUG( "log entry " << x << ": " << e.what() << "\n" << entry );
        }
        ++x;
    }
}


void dds_device::impl::open( const dds_stream_profiles & profiles )
{
    if( profiles.empty() )
        DDS_THROW( runtime_error, "must provide at least one profile" );

    auto stream_profiles = nlohmann::json();
    for( auto & profile : profiles )
    {
        auto stream = profile->stream();
        if( ! stream )
            DDS_THROW( runtime_error, "profile " << profile->to_string() << " is not part of any stream" );
        if( stream_profiles.find( stream->name() ) != stream_profiles.end() )
            DDS_THROW( runtime_error, "more than one profile found for stream '" << stream->name() << "'" );

        stream_profiles[stream->name()] = profile->to_json();
    }

    nlohmann::json j = {
        { id_key, id_open_streams },
        { "stream-profiles", stream_profiles },
    };

    nlohmann::json reply;
    write_control_message( j, &reply );

    if( rsutils::json::get( reply, status_key, status_ok ) != status_ok )
        throw std::runtime_error( "failed to open stream: "
                                  + rsutils::json::get< std::string >( reply, explanation_key, "no explanation" ) );
}


void dds_device::impl::set_option_value( const std::shared_ptr< dds_option > & option, float new_value )
{
    if( ! option )
        DDS_THROW( runtime_error, "must provide an option to set" );

    nlohmann::json j = nlohmann::json::object({
        { id_key, id_set_option },
        { option_name_key, option->get_name() },
        { value_key, new_value }
    });
    if( auto stream = option->stream() )
        j[stream_name_key] = stream->name();

    nlohmann::json reply;
    write_control_message( j, &reply );

    if( rsutils::json::get( reply, status_key, status_ok ) != status_ok )
        throw std::runtime_error(
            rsutils::json::get< std::string >( reply, explanation_key, "failed to set-option; no explanation" ) );
    //option->set_value( new_value );
}


float dds_device::impl::query_option_value( const std::shared_ptr< dds_option > & option )
{
    if( !option )
        DDS_THROW( runtime_error, "must provide an option to query" );

    nlohmann::json j = nlohmann::json::object({
        { id_key, id_query_option },
        { option_name_key, option->get_name() }
    });
    if( auto stream = option->stream() )
        j[stream_name_key] = stream->name();

    nlohmann::json reply;
    write_control_message( j, &reply );

    if( rsutils::json::get( reply, status_key, status_ok ) != status_ok )
        throw std::runtime_error(
            rsutils::json::get< std::string >( reply, explanation_key, "failed to query-option; no explanation" ) );
    return rsutils::json::get< float >( reply, value_key );
}


void dds_device::impl::write_control_message( topics::flexible_msg && msg, nlohmann::json * reply )
{
    assert( _control_writer != nullptr );
    auto this_sequence_number = std::move( msg ).write_to( *_control_writer );
    if( reply )
    {
        std::unique_lock< std::mutex > lock( _replies_mutex );
        auto & actual_reply = _replies[this_sequence_number];  // create it; initialized to null json
        if( ! _replies_cv.wait_for( lock,
                                    std::chrono::milliseconds( _reply_timeout_ms ),
                                    [&]()
                                    {
                                        if( actual_reply.is_null() )
                                            return false;
                                        return true;
                                    } ) )
        {
            DDS_THROW( runtime_error, "timeout waiting for reply #" << this_sequence_number );
        }
        //LOG_DEBUG( "got reply: " << actual_reply );
        *reply = std::move( actual_reply );
        _replies.erase( this_sequence_number );
    }
}

void dds_device::impl::create_notifications_reader()
{
    if( _notifications_reader )
        return;

    auto topic = topics::flexible_msg::create_topic( _participant, _info.topic_root + topics::NOTIFICATION_TOPIC_NAME );

    // We have some complicated topic structures. In particular, the metadata topic is created on demand while handling
    // other notifications, which doesn't work well (deadlock) if the notification is not called from another thread. So
    // we need the notification handling on another thread:
    _notifications_reader = std::make_shared< dds_topic_reader_thread >( topic, _subscriber );

    dds_topic_reader::qos rqos( eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS );
    //On discovery writer sends a burst of messages, if history is too small we might loose some of them
    //(even if reliable). Setting depth to cover known use-cases plus some spare
    rqos.history().depth = 24;
    rqos.override_from_json( rsutils::json::nested( _device_settings, "notification" ) );

    _notifications_reader->on_data_available(
        [&]()
        {
            topics::flexible_msg notification;
            eprosima::fastdds::dds::SampleInfo sample;
            while( topics::flexible_msg::take_next( *_notifications_reader, &notification, &sample ) )
            {
                if( ! notification.is_valid() )
                    continue;
                auto j = notification.json_data();
                if( j.is_array() )
                {
                    for( unsigned x = 0; x < j.size(); ++x )
                        handle_notification( j[x], sample );
                }
                else
                {
                    handle_notification( j, sample );
                }
            }
        } );

    _notifications_reader->run( rqos );
}

void dds_device::impl::create_metadata_reader()
{
    if( _metadata_reader ) // We can be called multiple times, once per stream
        return;

    auto topic = topics::flexible_msg::create_topic( _participant, _info.topic_root + topics::METADATA_TOPIC_NAME );
    _metadata_reader = std::make_shared< dds_topic_reader_thread >( topic, _subscriber );
    _metadata_reader->on_data_available(
        [this]()
        {
            topics::flexible_msg message;
            while( topics::flexible_msg::take_next( *_metadata_reader, &message ) )
            {
                if( message.is_valid() && _on_metadata_available )
                {
                    try
                    {
                        _on_metadata_available( std::move( message.json_data() ) );
                    }
                    catch( std::exception const & e )
                    {
                        LOG_DEBUG( "metadata exception: " << e.what() );
                    }
                }
            }
        } );

    // NOTE: the metadata thread is only run() when we've reached the READY state
}

void dds_device::impl::create_control_writer()
{
    if( _control_writer )
        return;

    auto topic = topics::flexible_msg::create_topic( _participant, _info.topic_root + topics::CONTROL_TOPIC_NAME );
    _control_writer = std::make_shared< dds_topic_writer >( topic );
    dds_topic_writer::qos wqos( eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS );
    wqos.history().depth = 10;  // default is 1
    wqos.override_from_json( rsutils::json::nested( _device_settings, "control" ) );
    _control_writer->run( wqos );
}


void dds_device::impl::on_device_header( nlohmann::json const & j, eprosima::fastdds::dds::SampleInfo const & sample )
{
    if( _state != state_t::WAIT_FOR_DEVICE_HEADER )
        return;

    // The server GUID is the server's notification writer's GUID -- that way, we can easily associate all notifications
    // with a server.
    eprosima::fastrtps::rtps::iHandle2GUID( _server_guid, sample.publication_handle );

    _n_streams_expected = rsutils::json::get< size_t >( j, "n-streams" );
    LOG_DEBUG( "... " << id_device_header << ": " << _n_streams_expected << " streams expected" );

    if( rsutils::json::has( j, "extrinsics" ) )
    {
        for( auto & ex : j["extrinsics"] )
        {
            std::string from_name = rsutils::json::get< std::string >( ex, 0 );
            std::string to_name = rsutils::json::get< std::string >( ex, 1 );
            LOG_DEBUG( "    ... got extrinsics from " << from_name << " to " << to_name );
            extrinsics extr = extrinsics::from_json( rsutils::json::get< nlohmann::json >( ex, 2 ) );
            _extrinsics_map[std::make_pair( from_name, to_name )] = std::make_shared< extrinsics >( extr );
        }
    }

    set_state( state_t::WAIT_FOR_DEVICE_OPTIONS );
}


void dds_device::impl::on_device_options( nlohmann::json const & j, eprosima::fastdds::dds::SampleInfo const & sample )
{
    if( _state != state_t::WAIT_FOR_DEVICE_OPTIONS )
        return;

    if( rsutils::json::has( j, "options" ) )
    {
        LOG_DEBUG( "... " << id_device_options << ": " << j["options"].size() << " options received" );

        for( auto & option_json : j["options"] )
        {
            auto option = dds_option::from_json( option_json );
            _options.push_back( option );
        }
    }

    if( _n_streams_expected )
        set_state( state_t::WAIT_FOR_STREAM_HEADER );
    else
        set_state( state_t::READY );
}


void dds_device::impl::on_stream_header( nlohmann::json const & j, eprosima::fastdds::dds::SampleInfo const & sample )
{
    if( _state != state_t::WAIT_FOR_STREAM_HEADER )
        return;

    if( _streams.size() >= _n_streams_expected )
        DDS_THROW( runtime_error, "more streams than expected (" << _n_streams_expected << ") received" );
    auto stream_type = rsutils::json::get< std::string >( j, "type" );
    auto stream_name = rsutils::json::get< std::string >( j, "name" );

    auto & stream = _streams[stream_name];
    if( stream )
        DDS_THROW( runtime_error, "stream '" << stream_name << "' already exists" );

    auto sensor_name = rsutils::json::get< std::string >( j, "sensor-name" );
    size_t default_profile_index = rsutils::json::get< size_t >( j, "default-profile-index" );
    dds_stream_profiles profiles;

#define TYPE2STREAM( S, P )                                                                                            \
    if( stream_type == #S )                                                                                            \
    {                                                                                                                  \
        for( auto & profile : j["profiles"] )                                                                          \
            profiles.push_back( dds_stream_profile::from_json< dds_##P##_stream_profile >( profile ) );                \
        stream = std::make_shared< dds_##S##_stream >( stream_name, sensor_name );                                     \
    }                                                                                                                  \
    else

    TYPE2STREAM( depth, video )
    TYPE2STREAM( ir, video )
    TYPE2STREAM( color, video )
    TYPE2STREAM( motion, motion )
    TYPE2STREAM( confidence, video )
    DDS_THROW( runtime_error, "stream '" << stream_name << "' is of unknown type '" << stream_type << "'" );

#undef TYPE2STREAM

    if( rsutils::json::get< bool >( j, "metadata-enabled" ) )
    {
        create_metadata_reader();
        stream->enable_metadata();  // Call before init_profiles
    }

    if( default_profile_index < profiles.size() )
        stream->init_profiles( profiles, default_profile_index );
    else
        DDS_THROW( runtime_error,
                   "stream '" << stream_name << "' default profile index " << default_profile_index
                              << " is out of bounds" );
    if( strcmp( stream->type_string(), stream_type.c_str() ) != 0 )
        DDS_THROW( runtime_error,
                   "failed to instantiate stream type '" << stream_type << "' (instead, got '" << stream->type_string()
                                                         << "')" );

    LOG_DEBUG( "... stream " << _streams.size() << "/" << _n_streams_expected << " '" << stream_name
                             << "' received with " << profiles.size() << " profiles"
                             << ( stream->metadata_enabled() ? " and metadata" : "" ) );

    set_state( state_t::WAIT_FOR_STREAM_OPTIONS );
}


void dds_device::impl::on_stream_options( nlohmann::json const & j, eprosima::fastdds::dds::SampleInfo const & sample )
{
    if( _state != state_t::WAIT_FOR_STREAM_OPTIONS )
        return;

    auto stream_it = _streams.find( rsutils::json::get< std::string >( j, "stream-name" ) );
    if( stream_it == _streams.end() )
        DDS_THROW( runtime_error,
                   std::string( "Received stream options for stream '" )
                       + rsutils::json::get< std::string >( j, "stream-name" )
                       + "' whose header was not received yet" );

    if( rsutils::json::has( j, "options" ) )
    {
        dds_options options;
        for( auto & option : j["options"] )
        {
            options.push_back( dds_option::from_json( option ) );
        }

        stream_it->second->init_options( options );
    }

    auto intit = j.find( "intrinsics" );
    if( intit != j.end() )
    {
        nlohmann::json const & j_int = *intit;
        if( auto video_stream = std::dynamic_pointer_cast< dds_video_stream >( stream_it->second ) )
        {
            std::set< video_intrinsics > intrinsics;
            for( auto & intr : j_int )
                intrinsics.insert( video_intrinsics::from_json( intr ) );
            video_stream->set_intrinsics( std::move( intrinsics ) );
        }
        else if( auto motion_stream = std::dynamic_pointer_cast< dds_motion_stream >( stream_it->second ) )
        {
            motion_stream->set_accel_intrinsics( motion_intrinsics::from_json( j_int["accel"] ) );
            motion_stream->set_gyro_intrinsics( motion_intrinsics::from_json( j_int["gyro"] ) );
        }
    }

    if( rsutils::json::has( j, "recommended-filters" ) )
    {
        std::vector< std::string > filter_names;
        for( auto & filter : j["recommended-filters"] )
        {
            filter_names.push_back( filter );
        }

        stream_it->second->set_recommended_filters( std::move( filter_names ) );
    }

    if( _streams.size() >= _n_streams_expected )
        set_state( state_t::READY );
    else
        set_state( state_t::WAIT_FOR_STREAM_HEADER );
}


}  // namespace realdds
