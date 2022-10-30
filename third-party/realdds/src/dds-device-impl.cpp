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

std::ostream & operator<<( std::ostream & s, state_type st )
{
    switch( st )
    {
    case state_type::WAIT_FOR_DEVICE_HEADER:
        s << "WAIT_FOR_DEVICE_HEADER";
        break;
    case state_type::WAIT_FOR_PROFILES:
        s << "WAIT_FOR_PROFILES";
        break;
    case state_type::DONE:
        s << "DONE";
        break;
    default:
        s << "UNKNOWN";
        break;
    }
    return s;
}


std::shared_ptr< dds_stream_profile >
to_realdds_profile( const topics::device::notification::video_stream_profile & profile )
{
    auto prof = std::make_shared< dds_video_stream_profile >( dds_stream_uid( profile.uid, profile.stream_index ),
                                                              dds_stream_format::from_rs2( profile.format ),
                                                              profile.framerate,
                                                              profile.width,
                                                              profile.height,
                                                              0, // TODO - bpp
                                                              profile.type);
    // TODO - add intrinsics

    return prof;
}

std::shared_ptr< dds_stream_profile >
to_realdds_profile( const topics::device::notification::motion_stream_profile & profile )
{
    auto prof = std::make_shared< dds_motion_stream_profile >( dds_stream_uid( profile.uid, profile.stream_index ),
                                                               dds_stream_format::from_rs2( profile.format ),
                                                               profile.framerate,
                                                               profile.type );
    // TODO - add intrinsics

    return prof;
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

    auto topic = topics::device::notification::create_topic( _participant, _info.topic_root + "/notification" );

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

    while( ! t.has_expired() && state_type::DONE != state )
    {
        LOG_DEBUG( state << "..." );
        eprosima::fastrtps::Duration_t one_second = { 1, 0 };
        if( _notifications_reader->get()->wait_for_unread_message( one_second ) )
        {
            topics::device::notification data;
            eprosima::fastdds::dds::SampleInfo info;
            while( topics::device::notification::take_next( *_notifications_reader, &data, &info ) )
            {
                if( ! data.is_valid() )
                    continue;

                switch( data._msg_type )
                {
                case topics::device::notification::msg_type::DEVICE_HEADER:
                    if( state_type::WAIT_FOR_DEVICE_HEADER == state )
                    {
                        auto device_header = data.get< topics::device::notification::device_header_msg >();
                        if( ! device_header )
                        {
                            LOG_ERROR( "Got DEVICE_HEADER with no data" );
                            break;
                        }
                        _expected_num_of_streams = device_header->num_of_streams;
                        LOG_INFO( "... DEVICE_HEADER: " << _expected_num_of_streams << " streams" );

                        if( _expected_num_of_streams )
                            state = state_type::WAIT_FOR_PROFILES;
                        else
                            state = state_type::DONE;
                    }
                    else
                        LOG_ERROR( "... DEVICE_HEADER unexpected" );
                    break;

                case topics::device::notification::msg_type::VIDEO_STREAM_PROFILES:
                    if( state_type::WAIT_FOR_PROFILES == state )
                    {
                        auto video_stream_profiles
                            = data.get< topics::device::notification::video_stream_profiles_msg >();
                        if( ! video_stream_profiles )
                        {
                            LOG_ERROR( "Got VIDEO_STREAM_PROFILES with no data" );
                            break;
                        }
                        if( _streams.size() >= _expected_num_of_streams )
                            DDS_THROW( runtime_error,
                                       "more streams than expected (" + std::to_string( _expected_num_of_streams )
                                           + ") received" );

                        std::string stream_name = video_stream_profiles->group_name;  // todo
                        std::string sensor_name = stream_name;  // todo
                        auto & stream = _streams[stream_name];
                        if( stream )
                            DDS_THROW( runtime_error, "stream '" + stream_name + "' already exists" );
                        stream = std::make_shared< dds_video_stream >( stream_name, sensor_name );
                        dds_stream_profiles profiles;
                        int default_profile_index = 0;
                        for( int i = 0; i < video_stream_profiles->num_of_profiles; ++i )
                        {
                            auto const & profile = video_stream_profiles->profiles[i];
                            profiles.push_back( to_realdds_profile( profile ) );
                            if( profile.default_profile )
                                default_profile_index = i;
                        }
                        stream->init_profiles( profiles, default_profile_index );
                        LOG_INFO( "... VIDEO_STREAM_PROFILES: " << _streams.size() << "/" << _expected_num_of_streams
                                                                << " streams received" );
                        if( _streams.size() >= _expected_num_of_streams )
                            state = state_type::DONE;
                    }
                    else
                        LOG_ERROR( "... VIDEO_STREAM_PROFILES unexpected" );
                    break;

                case topics::device::notification::msg_type::MOTION_STREAM_PROFILES:
                    if( state_type::WAIT_FOR_PROFILES == state )
                    {
                        auto motion_stream_profiles
                            = data.get< topics::device::notification::motion_stream_profiles_msg >();
                        if( ! motion_stream_profiles )
                        {
                            LOG_ERROR( "Got MOTION_STREAM_PROFILES with no data" );
                            break;
                        }
                        if( _streams.size() >= _expected_num_of_streams )
                            DDS_THROW( runtime_error,
                                       "more streams than expected (" + std::to_string( _expected_num_of_streams )
                                           + ") received" );

                        std::string stream_name = motion_stream_profiles->group_name;  // todo
                        std::string sensor_name = stream_name;                         // todo
                        auto & stream = _streams[stream_name];
                        if( stream )
                            DDS_THROW( runtime_error, "stream '" + stream_name + "' already exists" );
                        stream = std::make_shared< dds_motion_stream >( stream_name, sensor_name );
                        dds_stream_profiles profiles;
                        int default_profile_index = 0;
                        for( int i = 0; i < motion_stream_profiles->num_of_profiles; ++i )
                        {
                            auto const & profile = motion_stream_profiles->profiles[i];
                            profiles.push_back( to_realdds_profile( profile ) );
                            if( profile.default_profile )
                                default_profile_index = i;
                        }
                        stream->init_profiles( profiles, default_profile_index );
                        LOG_INFO( "... MOTION_STREAM_PROFILES: " << _streams.size() << "/" << _expected_num_of_streams
                                                                 << " streams received" );
                        if( _streams.size() >= _expected_num_of_streams )
                            state = state_type::DONE;
                    }
                    else
                        LOG_ERROR( "... MOTION_STREAM_PROFILES unexpected" );
                    break;

                default:
                    LOG_ERROR( "... Wrong message (" << (int)data._msg_type << ") received" );
                    break;
                }
            }
        }
    }
    if( state_type::DONE != state )
        LOG_DEBUG( "timed out; state is " << state );
    return ( state_type::DONE == state );
}


}  // namespace realdds
