// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-device.h>
#include <realdds/dds-device-impl.h>

#include <set>

namespace realdds {


namespace {

// The map from guid to device is global
typedef std::map< dds_guid, std::weak_ptr< dds_device > > guid_to_device_map;
guid_to_device_map guid_to_device;
std::mutex devices_mutex;

}  // namespace


dds_device::dds_stream::dds_stream( rs2_stream type, std::string group_name )
    : _impl(std::make_shared< dds_stream::impl >( type, group_name ))
{
}

void dds_device::dds_stream::add_video_profile( const rs2_video_stream & profile, bool default_profile )
{
    if ( _impl->_type != profile.type )
    {
        throw std::runtime_error( "profile of different type then stream" );
    }

    _impl->_video_profiles.push_back( { profile, default_profile } );
}

void dds_device::dds_stream::add_motion_profile( const rs2_motion_stream & profile, bool default_profile )
{
    if ( _impl->_type != profile.type )
    {
        throw std::runtime_error( "profile of different type then stream" );
    }

    _impl->_motion_profiles.push_back( { profile, default_profile } );
}

size_t dds_device::dds_stream::foreach_video_profile( std::function< void( const rs2_video_stream & profile, bool def_prof ) > fn ) const
{
    for ( auto profile : _impl->_video_profiles )
    {
        fn( profile.first, profile.second );
    }

    return _impl->_video_profiles.size();
}

size_t dds_device::dds_stream::foreach_motion_profile( std::function< void( const rs2_motion_stream & profile, bool def_prof ) > fn ) const
{
    for ( auto profile : _impl->_motion_profiles )
    {
        fn( profile.first, profile.second );
    }

    return _impl->_motion_profiles.size();
}

//std::string dds_device::dds_stream::get_name()
//{
//    std::stringstream ss;
//    ss << rs2_stream_to_string( _impl->_type );
//
//    //Add index number to the name
//    if ( _impl->_type == RS2_STREAM_DEPTH || _impl->_type == RS2_STREAM_COLOR || _impl->_type == RS2_STREAM_INFRARED )
//    {
//        if ( ! _impl->_video_profiles.empty() && _impl->_video_profiles[0].first.index != 0 )
//        {
//            ss << " " << _impl->_video_profiles[0].first.index;
//        }
//    }
//    else if ( _impl->_type == RS2_STREAM_GYRO || _impl->_type == RS2_STREAM_ACCEL )
//    {
//        if ( ! _impl->_motion_profiles.empty() && _impl->_motion_profiles[0].first.index != 0 )
//        {
//            ss << " " << _impl->_motion_profiles[0].first.index;
//        }
//    }
//    else
//    {
//        throw std::runtime_error( "trying to get name of unsupported stream type " + std::to_string( _impl->_type ) );
//    }
//
//    return ss.str();
//}

std::shared_ptr< dds_device > dds_device::find( dds_guid const & guid )
{
    return find_internal( guid, true );
}

std::shared_ptr< dds_device > dds_device::find_internal( dds_guid const & guid, bool shouldLock )
{
    std::shared_ptr< dds_device > dev;

    if( shouldLock )
        devices_mutex.lock();

    auto it = guid_to_device.find( guid );
    if( it != guid_to_device.end() )
    {
        dev = it->second.lock();
        if( ! dev )
        {
            // The device is no longer in use; clear it out
            guid_to_device.erase( it );
        }
    }

    if ( shouldLock )
        devices_mutex.unlock();

    return dev;
}

std::shared_ptr< dds_device > dds_device::create( std::shared_ptr< dds_participant > const & participant,
                                                  dds_guid const & guid,
                                                  topics::device_info const & info )
{
    // Locking before find_internal() to avoid a device created between search and creation
    std::lock_guard< std::mutex > lock( devices_mutex );

    std::shared_ptr< dds_device > dev = find_internal( guid, false );

    if( ! dev )
    {
        auto impl = std::make_shared< dds_device::impl >( participant, guid, info );
        // Use a custom deleter to automatically remove the device from the map when it's done with
        dev = std::shared_ptr< dds_device >( new dds_device( impl ), [guid]( dds_device * ptr ) {
            std::lock_guard< std::mutex > lock( devices_mutex );
            guid_to_device.erase( guid );
            delete ptr;
        } );
        guid_to_device.emplace( guid, dev );
    }

    return dev;
}

dds_device::dds_device( std::shared_ptr< impl > impl )
    : _impl( impl )
{
}

bool dds_device::is_running() const
{
    return _impl->_running;
}

void dds_device::run()
{
    _impl->run();
}

topics::device_info const & dds_device::device_info() const
{
    return _impl->_info;
}

dds_guid const & dds_device::guid() const
{
    return _impl->_guid;
}

size_t dds_device::num_of_streams() const
{
    return _impl->_streams.size();
}

size_t dds_device::num_of_stream_groups() const
{
    return _impl->_streams_in_group.size();
}

//size_t dds_device::foreach_stream( std::function< void( const std::string & name ) > fn ) const
//{
//    for ( auto const & stream : _impl->_streams )
//    {
//        fn( stream.second->get_name() );
//    }
//
//    return _impl->_streams.size();
//}

size_t dds_device::foreach_stream_group( std::function< void( const std::string & name ) > fn ) const
{
    for ( auto const & group : _impl->_streams_in_group )
    {
        fn( group.first );
    }

    return _impl->_streams_in_group.size();
}

size_t
dds_device::foreach_video_profile( std::function< void( const rs2_video_stream & profile, bool def_prof ) > fn ) const
{
    size_t profiles_num = 0;

    for ( auto const & stream : _impl->_streams )
    {
        profiles_num += stream.second->foreach_video_profile( fn );
    }

    return profiles_num;
}

size_t
dds_device::foreach_motion_profile( std::function< void( const rs2_motion_stream & profile, bool def_prof ) > fn ) const
{
    size_t profiles_num = 0;

    for ( auto const & stream : _impl->_streams )
    {
        profiles_num += stream.second->foreach_motion_profile( fn );
    }

    return profiles_num;
}

size_t
dds_device::foreach_video_profile_in_group( const std::string & group_name,
                                            std::function< void( const rs2_video_stream & profile, bool def_prof ) > fn ) const
{
    auto group = _impl->_streams_in_group.find( group_name );
    if( group != _impl->_streams_in_group.end() )
    for ( auto const & stream : group->second )
    {
        stream->foreach_video_profile( fn );
    }

    return group->second.size();
}

size_t
dds_device::foreach_motion_profile_in_group( const std::string & group_name,
                                             std::function< void( const rs2_motion_stream & profile, bool def_prof ) > fn ) const
{
    auto group = _impl->_streams_in_group.find( group_name );
    if( group != _impl->_streams_in_group.end() )
    for ( auto const & stream : group->second )
    {
        stream->foreach_motion_profile( fn );
    }

    return group->second.size();
}

void dds_device::open( const std::vector< rs2_video_stream > & profiles )
{
    using namespace topics;

    if (profiles.size() > device::control::MAX_OPEN_PROFILES)
    {
        throw std::runtime_error( "Too many profiles to open (" + std::to_string( profiles.size() )
                                + "), max is " + std::to_string( device::control::MAX_OPEN_PROFILES ) );
    }

    device::control::profiles_open_msg open_msg;
    open_msg.message_id = _impl->_control_message_counter++;
    for ( size_t i = 0; i < profiles.size(); ++i )
    {
        open_msg.profiles[i].uid = profiles[i].uid;
        open_msg.profiles[i].framerate = profiles[i].fps;
        open_msg.profiles[i].format = profiles[i].fmt;
        open_msg.profiles[i].type = profiles[i].type;
        open_msg.profiles[i].width = profiles[i].width;
        open_msg.profiles[i].height = profiles[i].height;
    }

    raw::device::control raw_msg;
    device::control::construct_raw_message( device::control::control_type::PROFILES_OPEN,
                                            open_msg,
                                            raw_msg );

    if ( _impl->write_control_message( &raw_msg ) )
    {
        LOG_DEBUG( "Sent PROFILES_OPEN message for " << profiles.size() << " profiles" ); // sensor " << sensor_index );
    }
    else
    {
        LOG_ERROR( "Error writing PROFILES_OPEN message for " << profiles.size() << " profiles" ); // sensor " << sensor_index );
    }
}

void dds_device::close( const std::vector< int16_t >& profile_uids )
{
    using namespace topics;

    if (profile_uids.size() > device::control::MAX_OPEN_PROFILES)
    {
        throw std::runtime_error( "Too many profiles to close (" + std::to_string( profile_uids.size() )
                                + "), max is " + std::to_string( device::control::MAX_OPEN_PROFILES ) );
    }

    device::control::profiles_close_msg close_msg;
    close_msg.message_id = _impl->_control_message_counter++;
    for (size_t i = 0; i < profile_uids.size(); ++i)
    {
        close_msg.profile_uids[i] = profile_uids[i];
    }

    raw::device::control raw_msg;
    device::control::construct_raw_message( device::control::control_type::PROFILES_CLOSE,
                                            close_msg,
                                            raw_msg );

    if (_impl->write_control_message( &raw_msg ))
    {
        LOG_DEBUG( "Sent PROFILES_CLOSE message for " << profile_uids.size() << " profiles" );
    }
    else
    {
        LOG_ERROR( "Error writing PROFILES_CLOSE message for " << profile_uids.size() << " profiles" );
    }
}


}  // namespace realdds
