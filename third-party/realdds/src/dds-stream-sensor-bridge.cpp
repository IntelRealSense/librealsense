// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <realdds/dds-stream-sensor-bridge.h>
#include <realdds/dds-device-server.h>
#include <realdds/dds-stream-server.h>
#include <realdds/dds-exceptions.h>

#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/json.h>

#include <algorithm>
#include <iostream>

using nlohmann::json;


namespace realdds {


dds_stream_sensor_bridge::dds_stream_sensor_bridge() {}


dds_stream_sensor_bridge::~dds_stream_sensor_bridge() {}


void dds_stream_sensor_bridge::init( std::vector< std::shared_ptr< dds_stream_server > > const & streams )
{
    if( streams.empty() )
        DDS_THROW( runtime_error, "no streams" );
    if( ! _sensors.empty() )
        DDS_THROW( runtime_error, "already initialized" );

    for( auto & server : streams )
    {
        stream_bridge & stream = _sensors[server->sensor_name()].streams[server->name()];  // create it
        stream.server = server;
        stream.profile = server->default_profile();
        server->on_readers_changed(
            [this]( std::shared_ptr< realdds::dds_stream_server > const & server, int n_readers )
            {
                stream_bridge & stream = _sensors[server->sensor_name()].streams[server->name()];
                on_streaming_needed( stream, n_readers > 0 );
                if( _on_readers_changed )
                    _on_readers_changed( server, n_readers );
            } );
    }
}


void dds_stream_sensor_bridge::on_streaming_needed( stream_bridge & stream, bool streaming_needed )
{
    if( streaming_needed != stream.is_streaming )
    {
        try
        {
            if( streaming_needed )
                // We have readers but are not streaming
                open( stream.profile );
            else
                // We are streaming but have no readers
                close( stream.server );
            stream.is_streaming = streaming_needed;  // before commit!
            commit();
        }
        catch( std::exception const & e )
        {
            std::string error_string
                = "failure trying to start/stop '" + stream.server->name() + "': " + e.what();
            LOG_ERROR( error_string );
            if( _on_error )
                _on_error( error_string );
        }
    }
}


bool profiles_are_compatible( std::shared_ptr< dds_stream_profile > const & p1,
                              std::shared_ptr< dds_stream_profile > const & p2,
                              bool any_format = false )
{
    auto vp1 = std::dynamic_pointer_cast< realdds::dds_video_stream_profile >( p1 );
    auto vp2 = std::dynamic_pointer_cast< realdds::dds_video_stream_profile >( p2 );
    if( ! ! vp1 != ! ! vp2 )
        return false;  // types aren't the same
    if( vp1 && vp2 )
        if( vp1->width() != vp2->width() || vp1->height() != vp2->height() )
            return false;
    if( ! any_format && p1->format() != p2->format() )
        return false;
    return p1->frequency() == p2->frequency();
}


static std::shared_ptr< realdds::dds_stream_profile >
find_profile( std::shared_ptr< realdds::dds_stream_server > const & stream,
              std::shared_ptr< realdds::dds_stream_profile > const & profile,
              bool any_format = false )
{
    auto & stream_profiles = stream->profiles();
    auto it = std::find_if( stream_profiles.begin(),
                            stream_profiles.end(),
                            [&]( std::shared_ptr< realdds::dds_stream_profile > const & sp )
                            { return profiles_are_compatible( sp, profile, any_format ); } );
    std::shared_ptr< realdds::dds_stream_profile > found_profile;
    if( it != stream_profiles.end() )
        found_profile = *it;
    return found_profile;
}


std::shared_ptr< realdds::dds_stream_server >
dds_stream_sensor_bridge::find_server( std::shared_ptr< realdds::dds_stream_profile > const & profile )
{
    if( ! profile )
        DDS_THROW( runtime_error, "null profile" );
    
    auto server = profile->stream();
    if( ! server )
        DDS_THROW( runtime_error, "profile " + profile->to_string() + " has no stream" );

    auto sensorit = _sensors.find( server->sensor_name() );
    if( sensorit == _sensors.end() )
        DDS_THROW( runtime_error, "no sensor '" + server->sensor_name() + "'" );
    auto & sensor = sensorit->second;

    auto streamit = sensor.streams.find( server->name() );
    if( streamit == sensor.streams.end() )
        DDS_THROW( runtime_error, "no stream '" + server->name() + "'" );
    auto & stream = streamit->second;

    if( stream.server != server )
        DDS_THROW( runtime_error, "profile stream '" + server->name() + "' does not match server" );
    return stream.server;
}


void dds_stream_sensor_bridge::reset( bool by_force )
{
    for( auto & name2sensor : _sensors )
    {
        auto & sensor = name2sensor.second;
        if( sensor.is_streaming )
        {
            if( ! by_force )
            {
                LOG_WARNING( "cannot reset streaming sensor '" << name2sensor.first << "'" );
                continue;  // We cannot reset something that's streaming!
            }
            stop_sensor( name2sensor.first, sensor );
        }

        for( auto & name2stream : sensor.streams )
        {
            auto & stream = name2stream.second;
            stream.profile = stream.server->default_profile();
            stream.is_explicit = false;
        }
    }
}


void dds_stream_sensor_bridge::open( std::shared_ptr< realdds::dds_stream_profile > const & profile )
{
    auto server = find_server( profile );
    auto & sensor = _sensors[server->sensor_name()];
    auto & stream = sensor.streams[server->name()];

    if( sensor.is_streaming && ! stream.is_on )
        DDS_THROW( runtime_error, "stream '" + server->name() + "' was not opened by the sensor" );

    // Check that the profile is compatible with any of the other already-open profiles
    for( auto & name2stream : sensor.streams )
    {
        stream_bridge & already_streaming = name2stream.second;
        if( already_streaming.is_on || already_streaming.is_explicit )
        {
            // Profiles are compatible if they match resolution, FPS
            if( ! profiles_are_compatible( profile, already_streaming.profile ) )
                DDS_THROW( runtime_error,
                           "profile " + profile->to_string() + " is incompatible with already-open "
                               + already_streaming.profile->to_string() );
        }
    }

    if( ! stream.is_explicit )
    {
        LOG_DEBUG( "using " << profile->to_string() );
        stream.profile = profile;
        stream.is_explicit = true;
    }
}


void dds_stream_sensor_bridge::close( std::shared_ptr< dds_stream_server > const & server )
{
    auto name2sensor = _sensors.find( server->sensor_name() );
    if( name2sensor == _sensors.end() )
        DDS_THROW( runtime_error, "invalid sensor name '" + server->sensor_name() + "'" );
    sensor_bridge & sensor = name2sensor->second;
    stream_bridge & stream = sensor.streams[server->name()];
    if( stream.is_explicit )
    {
        stream.is_explicit = false;
        if( ! stream.is_on )
        {
            stream.profile = stream.server->default_profile();
            LOG_DEBUG( "restoring stream '" << stream.server->name() << "' to default profile " << stream.profile->to_string() );
        }
    }
}


bool dds_stream_sensor_bridge::sensor_bridge::should_stream() const
{
    for( auto const & name2stream : streams )
    {
        auto & stream = name2stream.second;
        if( stream.is_streaming )
            return true;
    }
    return false;
}


template< class T >
std::ostream & operator<<( std::ostream & os, const std::shared_ptr< T > & p )
{
    if( !p )
        os << "(nullptr)";
    else
        os << *p;
    return os;
}


template< class T >
std::ostream & operator<<( std::ostream & os, const std::vector< T > & v )
{
    os << '[';
    bool first = true;
    for( auto & t : v )
    {
        if( first )
            first = false;
        else
            os << ' ';
        os << t;
    }
    os << ']';
    return os;
}


void dds_stream_sensor_bridge::commit()
{
    for( auto & name2sensor : _sensors )
    {
        auto & sensor = name2sensor.second;
        if( sensor.is_streaming == sensor.should_stream() )
            continue;
        sensor.is_streaming = ! sensor.is_streaming;
        if( sensor.is_streaming )
            start_sensor( name2sensor.first, sensor );
        else
            stop_sensor( name2sensor.first, sensor );
    }
}


void dds_stream_sensor_bridge::start_sensor( std::string const & sensor_name, sensor_bridge & sensor )
{
    auto active_profiles = finalize_profiles( sensor );
    LOG_DEBUG( "starting '" << sensor_name << "' with " << active_profiles );
    if( _on_start_sensor )
        _on_start_sensor( sensor_name, active_profiles );

    for( auto profile : active_profiles )
    {
        auto & stream = sensor.streams[profile->stream()->name()];
        stream.is_on = true;

        // Prepare the server with the right header (this will change when we remove "streaming" status from
        // stream-server)
        realdds::image_header header;
        header.format = profile->format();
        auto video_profile = std::dynamic_pointer_cast< realdds::dds_video_stream_profile >( profile );
        header.width = video_profile ? video_profile->width() : 0;
        header.height = video_profile ? video_profile->height() : 0;
        stream.server->start_streaming( header );
    }
}


dds_stream_profiles dds_stream_sensor_bridge::finalize_profiles( sensor_bridge & sensor )
{
    // Return a collection a profiles that would cover all streams, even implicit ones that have not been started
    std::vector< std::shared_ptr< dds_stream_profile > > active_profiles;

    // Fill in any missing implicit stream profiles
    for( auto & name2stream : sensor.streams )
    {
        stream_bridge & implicit_stream = name2stream.second;
        if( ! implicit_stream.is_explicit )
        {
            // Figure out a profile that would work with the others
            std::shared_ptr< realdds::dds_stream_profile > implicit_profile;
            std::shared_ptr< realdds::dds_stream_profile > implicit_profile_inexact;
            for( auto & name2stream2 : sensor.streams )
            {
                stream_bridge & streaming = name2stream2.second;
                if( streaming.is_streaming )
                {
                    implicit_profile = find_profile( implicit_stream.server, streaming.profile, false );  // same formats first
                    if( implicit_profile )
                        break;
                    if( ! implicit_profile_inexact )
                        implicit_profile_inexact = find_profile( implicit_stream.server, streaming.profile, true );  // allow different formats
                }
            }
            if( ! implicit_profile )
            {
                // Couldn't find an exact match -- pick the first inexact
                // (e.g., we're asked for depth at Z16 and have two other IRs at either Y8 or Y16 so they're not
                // compatible; pick the first one - the other will align to it)
                if( ! implicit_profile_inexact )
                {
                    LOG_WARNING( "inactive stream '" << name2stream.first << "' has no matching profile; it will not be streamable" );
                    implicit_stream.is_streaming = implicit_stream.is_on = false;  // just in case
                    continue;
                }
                implicit_profile = implicit_profile_inexact;
            }

            LOG_DEBUG( "using implicit " << implicit_profile->to_string() );
            implicit_stream.profile = implicit_profile;
            implicit_stream.is_on = true;
        }

        active_profiles.push_back( implicit_stream.profile );
    }

    return active_profiles;
}


void dds_stream_sensor_bridge::stop_sensor( std::string const & sensor_name, sensor_bridge & sensor )
{
    LOG_DEBUG( "stopping '" << sensor_name << "'" );
    if( _on_stop_sensor )
        _on_stop_sensor( sensor_name );

    for( auto & name2stream : sensor.streams )
    {
        auto & stream = name2stream.second;
        stream.is_on = stream.is_streaming = false;
        if( stream.server->is_streaming() )
            stream.server->stop_streaming();
        if( ! stream.is_explicit )
        {
            auto default_profile = stream.server->default_profile();
            if( stream.profile != default_profile )
            {
                LOG_DEBUG( "restoring implicit stream '" << name2stream.first << "' to default profile "
                                                         << default_profile->to_string() );
                stream.profile = default_profile;
            }
        }
    }
}


bool dds_stream_sensor_bridge::is_streaming( std::shared_ptr< dds_stream_server > const & server )
{
    auto name2sensor = _sensors.find( server->sensor_name() );
    if( name2sensor == _sensors.end() )
        DDS_THROW( runtime_error, "invalid sensor name '" + server->sensor_name() + "'" );
    sensor_bridge & sensor = name2sensor->second;
    stream_bridge & stream = sensor.streams[server->name()];
    return stream.is_streaming;
}


}  // namespace realdds
