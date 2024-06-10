// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

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

using rsutils::json;


namespace {


json device_settings( std::shared_ptr< realdds::dds_participant > const & participant )
{
    auto settings = participant->settings().nested( "device" );
    if( ! settings )
        // Nothing there: default is empty object
        return json::object();
    if( ! settings.is_object() )
        // Device settings, if they exist, must be an object!
        DDS_THROW( runtime_error, "participant 'device' settings must be an object: " << settings );
    return settings;
}


}


namespace realdds {


void dds_device::impl::set_state( state_t new_state )
{
    if( new_state == _state )
        return;

    if( state_t::READY == new_state )
    {
        if( _metadata_reader )
        {
            auto md_settings = _device_settings.nested( "metadata" );
            if( md_settings.exists() && ! md_settings.is_object() )
            {
                LOG_DEBUG( "[" << debug_name() << "] ... metadata is available but device/metadata is disabled" );
                _metadata_reader.reset();
            }
            else
            {
                LOG_DEBUG( "[" << debug_name() << "] ... metadata is enabled" );
                dds_topic_reader::qos rqos( eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS );
                rqos.history().depth = 10; // Support receive metadata from multiple streams
                rqos.override_from_json( md_settings );
                _metadata_reader->run( rqos );
            }
        }
        LOG_DEBUG( "[" << debug_name() << "] device is ready" );
    }

    _state = new_state;
}


dds_device::impl::impl( std::shared_ptr< dds_participant > const & participant,
                        topics::device_info const & info )
    : _info( info )
    , _participant( participant )
    , _subscriber( std::make_shared< dds_subscriber >( participant ) )
    , _device_settings( device_settings( participant ) )
    , _reply_timeout_ms(
          _device_settings.nested( "control", "reply-timeout-ms" ).default_value< size_t >( 2000 ) )
{
    create_control_writer();
    create_notifications_reader();
}


void dds_device::impl::reset()
{
    // _info should already be up-to-date
    // _participant doesn't change
    // _subscriber can stay the same
    // _reply_timeout_ms is using same settings

    // notifications/control/metadata topic, since the topic root hasn't changed, are still valid

    // Streams need to be reset
    _server_guid = {};
    _n_streams_expected = 0;
    _streams.clear();
    _options.clear();
    _extrinsics_map.clear();
    _metadata_reader.reset();
}


dds_guid const & dds_device::impl::guid() const
{
    return _control_writer->guid();
}


std::string dds_device::impl::debug_name() const
{
    return rsutils::string::from() << _info.debug_name() << _participant->print( guid() );
}


void dds_device::impl::on_notification( json && j, eprosima::fastdds::dds::SampleInfo const & notification_sample )
{
    typedef std::map< std::string,
                      void ( dds_device::impl::* )( json const &,
                                                    eprosima::fastdds::dds::SampleInfo const & ) >
        notification_handlers;
    static notification_handlers const _notification_handlers{
        { topics::reply::set_option::id, &dds_device::impl::on_set_option },
        { topics::reply::query_option::id, &dds_device::impl::on_set_option },  // Same handling as on_set_option
        { topics::reply::query_options::id, &dds_device::impl::on_query_options },
        { topics::notification::device_header::id, &dds_device::impl::on_device_header },
        { topics::notification::device_options::id, &dds_device::impl::on_device_options },
        { topics::notification::stream_header::id, &dds_device::impl::on_stream_header },
        { topics::notification::stream_options::id, &dds_device::impl::on_stream_options },
        { topics::notification::log::id, &dds_device::impl::on_log },
    };

    auto const control = j.nested( topics::reply::key::control );
    auto const control_sample = control ? j.nested( topics::reply::key::sample ) : rsutils::json_ref( rsutils::missing_json );

    try
    {
        // First handle the notification
        // An 'id' is mandatory, but if it's a response to a control it can be contained there
        auto id = ( control_sample ? control.get_json() : j ).nested( topics::notification::key::id ).string_ref();
        auto it = _notification_handlers.find( id );
        if( it != _notification_handlers.end() )
            ( this->*( it->second ) )( j, notification_sample );
        _on_notification.raise( id, j );
    }
    catch( std::exception const & e )
    {
        LOG_DEBUG( "[" << debug_name() << "] notification error: " << e.what() << "\n    " << j );
    }
    catch( ... )
    {
        LOG_DEBUG( "[" << debug_name() << "] notification error: unknown exception\n    " << j );
    }

    try
    {
        // Check if this is a reply - maybe someone's waiting on it...
        if( control_sample )
        {
            // ["<prefix>.<entity>", <sequence-number>]
            if( control_sample.size() == 2 && control_sample.is_array() )
            {
                // We have to be the ones who sent the control!
                auto const origin_guid = guid_from_string( control_sample[0].get< std::string >() );
                auto const control_guid = _control_writer->guid();
                if( origin_guid == control_guid )
                {
                    auto const sequence_number = control_sample[1].get< uint64_t >();
                    std::unique_lock< std::mutex > lock( _replies_mutex );
                    auto replyit = _replies.find( sequence_number );
                    if( replyit != _replies.end() )
                    {
                        replyit->second = std::move( j );
                        _replies_cv.notify_all();
                    }
                    else
                    {
                        // Nobody's waiting for it - but we can still log any errors:
                        dds_device::check_reply( j );
                    }
                }
            }
        }
    }
    catch( std::exception const & e )
    {
        LOG_DEBUG( "[" << debug_name() << "] reply error: " << e.what() << "  " << j );
    }
    catch( ... )
    {
        LOG_DEBUG( "[" << debug_name() << "] reply error: unknown exception  " << j );
    }
}


void dds_device::impl::on_set_option( json const & j, eprosima::fastdds::dds::SampleInfo const & )
{
    if( ! is_ready() )
        return;

    // This is the handler for "set-option" or "query-option", meaning someone sent a control request to set/get an
    // option value. In either case a value will be returned; we want to update our local copy to reflect it:

    std::string explanation;
    if( ! dds_device::check_reply( j, &explanation ) )
        return;  // we don't care about errors

    // We need the original control request as part of the reply, otherwise we can't know what option this is for
    auto control = j.nested( topics::reply::key::control );
    if( ! control.is_object() )
        throw std::runtime_error( "missing control object" );

    // Find the relevant (stream) options to update
    dds_options const * options = &_options;
    std::string const & stream_name =  // default = empty = device option
        control.nested( topics::control::set_option::key::stream_name ).string_ref_or_empty();
    if( ! stream_name.empty() )
    {
        auto stream_it = _streams.find( stream_name );
        if( stream_it == _streams.end() )
            throw std::runtime_error( "stream '" + stream_name + "' not found" );
        options = &stream_it->second->options();
    }

    auto value_j = j.nested( topics::reply::set_option::key::value );
    if( ! value_j.exists() )
        throw std::runtime_error( "missing value" );

    auto option_name_j = control.nested( topics::control::set_option::key::option_name );
    if( ! option_name_j.is_string() )
        throw std::runtime_error( "missing option-name" );
    auto & option_name = option_name_j.string_ref();
    for( auto & option : *options )
    {
        if( option->get_name() == option_name )
        {
            option->set_value( value_j );  // throws!
            return;
        }
    }
    throw std::runtime_error( "option '" + option_name + "' not found" );
}


void dds_device::impl::on_query_options( json const & j, eprosima::fastdds::dds::SampleInfo const & )
{
    if( ! is_ready() )
        return;

    // This is the notification for "query-options", which can get sent as a reply to a control or independently by the
    // device. It takes the same form & handling either way.
    // 
    // E.g.:
    //   {
    //     "id": "query-options",
    //     "option-values" : {
    //       "IP address": "1.2.3.4",  // device-level
    //       "Color": {
    //         "Exposure": 8.0,
    //       },
    //       "Depth" : {
    //         "Exposure": 20.0
    //       }
    //     }
    //   }

    dds_device::check_reply( j );  // throws

    // This little function is used either for device or stream options
    auto update_option = [this]( dds_options const & options, std::string const & option_name, json const & new_value )
    {
        // Find the option and set its value
        for( auto & option : options )
        {
            if( option->get_name() == option_name )
            {
                option->set_value( new_value );
                return;
            }
        }
        //LOG_DEBUG( "[" << debug_name() << "] option '" << option_name << "': not found" );
        throw std::runtime_error( "option '" + option_name + "' not found" );
    };

    auto option_values = j.nested( topics::reply::query_options::key::option_values );
    if( ! option_values.is_object() )
        throw std::runtime_error( "missing option-values" );

    //LOG_DEBUG( "[" << debug_name() << "] got query-options: " << option_values.dump(4) );
    for( auto it = option_values.begin(); it != option_values.end(); ++it )
    {
        if( it->is_object() )
        {
            // Stream name
            auto & stream_name = it.key();
            auto stream_it = _streams.find( stream_name );
            if( stream_it == _streams.end() )
                throw std::runtime_error( "stream '" + stream_name + "' not found" );
            auto & option_names = it.value();
            for( auto option_it = option_names.begin(); option_it != option_names.end(); ++option_it )
                update_option( stream_it->second->options(), option_it.key(), option_it.value() );
        }
        else
        {
            // Device-level option name
            update_option( _options, it.key(), it.value() );
        }
    }
}


void dds_device::impl::on_known_notification( json const & j, eprosima::fastdds::dds::SampleInfo const & )
{
    // This is a known notification, but we don't want to do anything for it
}


void dds_device::impl::on_log( json const & j, eprosima::fastdds::dds::SampleInfo const & )
{
    // This is the notification for "log"  (see docs/notifications.md#Logging)
    //     - `entries` is an array containing 1 or more log entries
    auto entries = j.nested( topics::notification::log::key::entries );
    if( ! entries )
        throw std::runtime_error( "log entries not found" );
    if( ! entries.is_array() )
        throw std::runtime_error( "log entries not an array" );
    // Each log entry is a JSON array of `[timestamp, type, text, data]` containing:
    //     - `timestamp`: when the event occurred
    //     - `type`: one of `EWID` (Error, Warning, Info, Debug)
    //     - `text`: any text that needs output
    //     - `data`: optional; an object containing any pertinent information about the event
    size_t x = 0;
    for( auto & entry : entries )
    {
        try
        {
            if( ! entry.is_array() )
                throw std::runtime_error( "not an array" );
            if( entry.size() < 3 || entry.size() > 4 )
                throw std::runtime_error( "bad array length" );
            auto timestamp = entry[0].get< dds_nsec >();
            auto const & stype = entry[1].string_ref();
            if( stype.length() != 1 || ! strchr( "EWID", stype[0] ) )
                throw std::runtime_error( "type not one of 'EWID'" );
            char const type = stype[0];
            auto const & text = entry[2].string_ref();
            auto const & data = entry.size() > 3 ? entry[3] : rsutils::null_json;

            if( ! _on_device_log.raise( timestamp, type, text, data ) )
                LOG_DEBUG( "[" << debug_name() << "][" << timestamp << "][" << type << "] " << text
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

    json stream_profiles;
    for( auto & profile : profiles )
    {
        auto stream = profile->stream();
        if( ! stream )
            DDS_THROW( runtime_error, "profile '" << profile->to_string() << "' is not part of any stream" );
        if( stream_profiles.nested( stream->name() ) )
            DDS_THROW( runtime_error, "more than one profile found for stream '" << stream->name() << "'" );

        stream_profiles[stream->name()] = profile->to_json();
    }

    json j = {
        { topics::control::key::id, topics::control::open_streams::id },
        { topics::control::open_streams::key::stream_profiles, std::move( stream_profiles ) },
    };

    json reply;
    write_control_message( j, &reply );
}


void dds_device::impl::set_option_value( const std::shared_ptr< dds_option > & option, json new_value )
{
    if( ! option )
        DDS_THROW( runtime_error, "must provide an option to set" );

    json j = json::object({
        { topics::control::key::id, topics::control::set_option::id },
        { topics::control::set_option::key::option_name, option->get_name() },
        { topics::control::set_option::key::value, new_value }
    });
    if( auto stream = option->stream() )
        j[topics::control::set_option::key::stream_name] = stream->name();

    json reply;
    write_control_message( j, &reply );
    // the reply will contain the new value (which may be different) and will update the cached one
}


json dds_device::impl::query_option_value( const std::shared_ptr< dds_option > & option )
{
    if( ! option )
        DDS_THROW( runtime_error, "must provide an option to query" );

    json j = json::object({
        { topics::control::key::id, topics::control::query_option::id },
        { topics::control::query_option::key::option_name, option->get_name() }
    });
    if( auto stream = option->stream() )
        j[topics::control::query_option::key::stream_name] = stream->name();

    json reply;
    write_control_message( j, &reply );

    return reply.at( topics::reply::query_option::key::value );
}


void dds_device::impl::write_control_message( topics::flexible_msg && msg, json * reply )
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

        // Throw if there's an error
        dds_device::check_reply( *reply );
    }
}

void dds_device::impl::create_notifications_reader()
{
    if( _notifications_reader )
        return;

    auto topic = topics::flexible_msg::create_topic( _participant, _info.topic_root() + topics::NOTIFICATION_TOPIC_NAME );

    // We have some complicated topic structures. In particular, the metadata topic is created on demand while handling
    // other notifications, which doesn't work well (deadlock) if the notification is not called from another thread. So
    // we need the notification handling on another thread:
    _notifications_reader = std::make_shared< dds_topic_reader_thread >( topic, _subscriber );

    dds_topic_reader::qos rqos( eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS );
    // On discovery writer sends a burst of messages, if history is too small we might lose some of them
    // (even if reliable). Setting depth to cover known use-cases plus some spare
    rqos.history().depth = 24;
    rqos.override_from_json( _device_settings.nested( "notification" ) );

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
                        on_notification( std::move( j[x] ), sample );
                }
                else
                {
                    on_notification( std::move( j ), sample );
                }
            }
        } );

    _notifications_reader->run( rqos );
}

void dds_device::impl::create_metadata_reader()
{
    if( _metadata_reader ) // We can be called multiple times, once per stream
        return;

    auto topic = topics::flexible_msg::create_topic( _participant, _info.topic_root() + topics::METADATA_TOPIC_NAME );
    _metadata_reader = std::make_shared< dds_topic_reader_thread >( topic, _subscriber );
    _metadata_reader->on_data_available(
        [this]()
        {
            topics::flexible_msg message;
            while( topics::flexible_msg::take_next( *_metadata_reader, &message ) )
            {
                if( message.is_valid() && _on_metadata_available.size() )
                {
                    try
                    {
                        auto sptr = std::make_shared< const json >( message.json_data() );
                        _on_metadata_available.raise( sptr );
                    }
                    catch( std::exception const & e )
                    {
                        LOG_DEBUG( "[" << debug_name() << "] metadata exception: " << e.what() );
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

    auto topic = topics::flexible_msg::create_topic( _participant, _info.topic_root() + topics::CONTROL_TOPIC_NAME );
    _control_writer = std::make_shared< dds_topic_writer >( topic );
    dds_topic_writer::qos wqos( eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS );
    wqos.history().depth = 10;  // default is 1
    wqos.override_from_json( _device_settings.nested( "control" ) );
    _control_writer->run( wqos );
}


void dds_device::impl::on_device_header( json const & j, eprosima::fastdds::dds::SampleInfo const & sample )
{
    if( _state != state_t::ONLINE )
        return;

    // We can get here when we regain connectivity - reset everything, just as if we're freshly constructed
    reset();

    // The server GUID is the server's notification writer's GUID -- that way, we can easily associate all notifications
    // with a server.
    eprosima::fastrtps::rtps::iHandle2GUID( _server_guid, sample.publication_handle );

    _n_streams_expected = j.at( topics::notification::device_header::key::n_streams ).get< size_t >();
    LOG_DEBUG( "[" << debug_name() << "] ... " << topics::notification::device_header::id << ": " << _n_streams_expected << " streams expected" );

    if( auto extrinsics_j = j.nested( topics::notification::device_header::key::extrinsics ) )
    {
        for( auto & ex : extrinsics_j )
        {
            std::string const & from_name = ex[0].string_ref();
            std::string const & to_name = ex[1].string_ref();
            LOG_DEBUG( "[" << debug_name() << "]     ... got extrinsics from " << from_name << " to " << to_name );
            extrinsics extr = extrinsics::from_json( ex[2] );
            _extrinsics_map[std::make_pair( from_name, to_name )] = std::make_shared< extrinsics >( extr );
        }
    }

    set_state( state_t::WAIT_FOR_DEVICE_OPTIONS );
}


void dds_device::impl::on_device_options( json const & j, eprosima::fastdds::dds::SampleInfo const & sample )
{
    if( _state != state_t::WAIT_FOR_DEVICE_OPTIONS )
        return;

    if( auto options_j = j.nested( topics::notification::device_options::key::options ) )
    {
        LOG_DEBUG( "[" << debug_name() << "] ... " << topics::notification::device_options::id << ": " << options_j.size() << " options received" );

        for( auto & option_json : options_j )
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


void dds_device::impl::on_stream_header( json const & j, eprosima::fastdds::dds::SampleInfo const & sample )
{
    if( _state != state_t::WAIT_FOR_STREAM_HEADER )
        return;

    if( _streams.size() >= _n_streams_expected )
        DDS_THROW( runtime_error, "more streams than expected (" << _n_streams_expected << ") received" );
    auto & stream_type = j.at( topics::notification::stream_header::key::type ).string_ref();
    auto & stream_name = j.at( topics::notification::stream_header::key::name ).string_ref();

    auto & stream = _streams[stream_name];
    if( stream )
        DDS_THROW( runtime_error, "stream '" << stream_name << "' already exists" );

    auto & sensor_name = j.at( topics::notification::stream_header::key::sensor_name ).string_ref();
    size_t default_profile_index = j.at( "default-profile-index" ).get< size_t >();
    dds_stream_profiles profiles;

#define TYPE2STREAM( S, P )                                                                                            \
    if( stream_type == #S )                                                                                            \
    {                                                                                                                  \
        for( auto & profile : j[topics::notification::stream_header::key::profiles] )                                  \
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

    if( j.at( topics::notification::stream_header::key::metadata_enabled ).get< bool >() )
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

    LOG_DEBUG( "[" << debug_name() << "] ... stream " << _streams.size() << "/" << _n_streams_expected << " '" << stream_name
                             << "' received with " << profiles.size() << " profiles"
                             << ( stream->metadata_enabled() ? " and metadata" : "" ) );

    set_state( state_t::WAIT_FOR_STREAM_OPTIONS );
}


void dds_device::impl::on_stream_options( json const & j, eprosima::fastdds::dds::SampleInfo const & sample )
{
    if( _state != state_t::WAIT_FOR_STREAM_OPTIONS )
        return;

    auto & stream_name = j.at( topics::notification::stream_options::key::stream_name ).string_ref();
    auto stream_it = _streams.find( stream_name );
    if( stream_it == _streams.end() )
        DDS_THROW( runtime_error,
                   "Received stream options for stream '" << stream_name << "' whose header was not received yet" );

    if( auto options_j = j.nested( topics::notification::stream_options::key::options ) )
    {
        dds_options options;
        for( auto & option_j : options_j )
        {
            LOG_DEBUG( "[" << debug_name() << "]     ... " << option_j );
            auto option = dds_option::from_json( option_j );
            options.push_back( option );
        }

        stream_it->second->init_options( options );
    }

    if( auto j_int = j.nested( topics::notification::stream_options::key::intrinsics ) )
    {
        if( auto video_stream = std::dynamic_pointer_cast< dds_video_stream >( stream_it->second ) )
        {
            std::set< video_intrinsics > intrinsics;
            for( auto & intr : j_int )
                intrinsics.insert( video_intrinsics::from_json( intr ) );
            video_stream->set_intrinsics( std::move( intrinsics ) );
        }
        else if( auto motion_stream = std::dynamic_pointer_cast< dds_motion_stream >( stream_it->second ) )
        {
            motion_stream->set_accel_intrinsics( motion_intrinsics::from_json(
                j_int.at( topics::notification::stream_options::intrinsics::key::accel ) ) );
            motion_stream->set_gyro_intrinsics( motion_intrinsics::from_json(
                j_int.at( topics::notification::stream_options::intrinsics::key::gyro ) ) );
        }
    }

    if( auto filters_j = j.nested( topics::notification::stream_options::key::recommended_filters ) )
    {
        std::vector< std::string > filter_names;
        for( auto & filter : filters_j )
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
