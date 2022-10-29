// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "dds-device-impl.h"

#include <realdds/dds-participant.h>
#include <realdds/dds-topic-reader.h>
#include <realdds/dds-topic-writer.h>

#include <realdds/topics/control/control-msg.h>
#include <librealsense2/utilities/time/timer.h>

#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>

#include <cassert>

#include <librealsense2/utilities/json.h>
using nlohmann::json;


namespace realdds {


namespace {

enum class state_type
{
    WAIT_FOR_DEVICE_HEADER,
    WAIT_FOR_PROFILES,
    DONE
    // TODO - Assuming all profiles of a stream will be sent in a single video_stream_profiles_msg
    // Otherwise need a stream header message with expected number of profiles for each stream
    // But then need all stream header messages to be sent before any profile message for a simple state machine
};

char const * to_string( state_type st )
{
    switch( st )
    {
    case state_type::WAIT_FOR_DEVICE_HEADER:
        return "WAIT_FOR_DEVICE_HEADER";
    case state_type::WAIT_FOR_PROFILES:
        return "WAIT_FOR_PROFILES";
    case state_type::DONE:
        return "DONE";
    default:
        return "UNKNOWN";
    }
}

std::ostream& operator<<( std::ostream& s, state_type st )
{
    s << to_string( st );
    return s;
}

}  // namespace


dds_device::impl::impl( std::shared_ptr< dds_participant > const & participant,
                        dds_guid const & guid,
                        topics::device_info const & info )
    : _info( info )
    , _guid( guid )
    , _participant( participant )
{
}

void dds_device::impl::run()
{
    if( _running )
        DDS_THROW( runtime_error, "device '" + _info.name + "' is already running" );

    create_notifications_reader();
    create_control_writer();
    if( ! init() )
        DDS_THROW( runtime_error, "failed getting stream data from '" + _info.topic_root + "'" );

    LOG_DEBUG( "device '" << _info.topic_root << "' (" << _participant->print( _guid )
                          << ") initialized successfully" );
    _running = true;
}

bool dds_device::impl::write_control_message( void * msg )
{
    assert( _control_writer != nullptr );

    return DDS_API_CALL( _control_writer->get()->write( msg ) );
}

void dds_device::impl::create_notifications_reader()
{
    if( _notifications_reader )
        return;

    auto topic = topics::notification::create_topic( _participant, _info.topic_root + "/notification" );

    _notifications_reader = std::make_shared< dds_topic_reader >( topic );
    _notifications_reader->run( dds_topic_reader::qos( eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS ) );
}

void dds_device::impl::create_control_writer()
{
    if( _control_writer )
        return;

    auto topic = topics::device::control::create_topic( _participant, _info.topic_root + "/control" );
    _control_writer = std::make_shared< dds_topic_writer >( topic );
    dds_topic_writer::qos wqos( eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS );
    wqos.history().depth = 10;  // default is 1
    _control_writer->run( wqos );
}

bool dds_device::impl::init()
{
    // We expect to receive all of the sensors data under a timeout
    utilities::time::timer t( std::chrono::seconds( 30 ) );  // TODO: refine time out
    state_type state = state_type::WAIT_FOR_DEVICE_HEADER;
    size_t n_streams_expected = 0;
    while( ! t.has_expired() && state_type::DONE != state )
    {
        LOG_DEBUG( state << "..." );
        eprosima::fastrtps::Duration_t one_second = { 1, 0 };
        if( _notifications_reader->get()->wait_for_unread_message( one_second ) )
        {
            topics::notification notification;
            eprosima::fastdds::dds::SampleInfo info;
            while( topics::notification::take_next( *_notifications_reader, &notification, &info ) )
            {
                if( ! notification.is_valid() )
                    continue;
                auto j = notification.json_data();
                auto id = j["id"].get< std::string >();
                if( state_type::WAIT_FOR_DEVICE_HEADER == state && id == "device-header" )
                {
                    n_streams_expected = utilities::json::get< size_t >( j, "n-streams" );
                    LOG_INFO( "... device-header: " << n_streams_expected << " streams expected" );
                    if( n_streams_expected )
                        state = state_type::WAIT_FOR_PROFILES;
                    else
                        state = state_type::DONE;
                }
                else if( state_type::WAIT_FOR_PROFILES == state && id == "stream-header" )
                {
                    if( _streams.size() >= n_streams_expected )
                        DDS_THROW( runtime_error,
                                   "more streams than expected (" + std::to_string( n_streams_expected )
                                       + ") received" );
                    auto type = utilities::json::get< std::string >( j, "type" );
                    auto stream_name = utilities::json::get< std::string >( j, "name" );
                    auto sensor_name = utilities::json::get< std::string >( j, "sensor-name" );
                    auto default_profile_index = utilities::json::get< int >( j, "default-profile-index" );
                    dds_stream_profiles profiles;
                    if( type == "video" )
                    {
                        for( auto profile : j["profiles"] )
                        {
                            auto frequency = utilities::json::get< int16_t >( profile, "frequency" );
                            dds_stream_format format( utilities::json::get< std::string >( profile, "format" ) );
                            auto width = utilities::json::get< int16_t >( profile, "width" );
                            auto height = utilities::json::get< int16_t >( profile, "height" );
                            profiles.push_back( std::make_shared< dds_video_stream_profile >(
                                dds_stream_uid( (uint32_t)_streams.size() ),
                                format,
                                frequency,
                                width,
                                height,
                                0 ) );  // bpp
                        }
                    }
                    else if( type == "motion" )
                    {
                        for( auto profile : j["profiles"] )
                        {
                            auto frequency = utilities::json::get< int16_t >( profile, "frequency" );
                            dds_stream_format format( utilities::json::get< std::string >( profile, "format" ) );
                            profiles.push_back( std::make_shared< dds_motion_stream_profile >(
                                dds_stream_uid( (uint32_t)_streams.size() ),
                                format,
                                frequency ) );
                        }
                    }
                    else
                        DDS_THROW( runtime_error, "stream '" + stream_name + "' is of unknown type '" + type + "'" );
                    if( default_profile_index < 0 || default_profile_index >= profiles.size() )
                        DDS_THROW( runtime_error,
                                   "stream '" + stream_name + "' default profile index "
                                       + std::to_string( default_profile_index ) + " is out of bounds" );

                    auto & stream = _streams[stream_name];
                    if( stream )
                        DDS_THROW( runtime_error, "stream '" + stream_name + "' already exists" );
                    stream = std::make_shared< dds_video_stream >( stream_name, sensor_name );
                    stream->init_profiles( profiles, default_profile_index );
                    LOG_INFO( "... stream '" << stream_name << "' (" << _streams.size() << "/" << n_streams_expected
                                             << ") received with " << profiles.size() << " profiles" );
                    if( _streams.size() >= n_streams_expected )
                        state = state_type::DONE;
                }
                else
                {
                    DDS_THROW( runtime_error, "unexpected notification '" + id + "' in " + to_string( state ) );
                }
            }
        }
    }
    if( state_type::DONE != state )
        LOG_DEBUG( "timed out; state is " << state );
    return ( state_type::DONE == state );
}


}  // namespace realdds
