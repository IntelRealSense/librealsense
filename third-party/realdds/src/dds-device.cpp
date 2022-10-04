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


std::shared_ptr< dds_device > dds_device::find( dds_guid const & guid )
{
    std::shared_ptr< dds_device > dev;

    std::lock_guard< std::mutex > lock( devices_mutex );
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

    return dev;
}


std::shared_ptr< dds_device > dds_device::create( std::shared_ptr< dds_participant > const & participant,
                                                  dds_guid const & guid,
                                                  topics::device_info const & info )
{
    std::shared_ptr< dds_device > dev = find( guid );
    // TODO a device may be created between find() and the next lock...
    if( ! dev )
    {
        std::lock_guard< std::mutex > lock( devices_mutex );

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


size_t dds_device::num_of_sensors() const
{
    std::set< std::string > sensors;
    for (auto p : _impl->_profile_to_profile_group)
    {
        sensors.insert( p.second );
    }

    return sensors.size();
}


size_t dds_device::foreach_sensor( std::function< void( const std::string & name ) > fn ) const
{
    std::set< std::string > sensors;
    for (auto p : _impl->_profile_to_profile_group)
    {
        sensors.insert( p.second );
    }

    for( auto sensor : sensors)
    {
        fn( sensor );
    }

    return sensors.size();
}


size_t
dds_device::foreach_video_profile( std::string group_name,
                                   std::function< void( const rs2_video_stream & profile, bool def_prof ) > fn ) const
{
    auto const & profiles = _impl->_video_profile_indexes_in_profile_group.find( group_name );

    if(profiles == _impl->_video_profile_indexes_in_profile_group.end() )
    {
        return 0;  // Not video profiles for this group, nothing to do
    }

    for( size_t i = 0; i < profiles->second.size(); ++i )
    {
        auto const & profile = _impl->_video_profiles[profiles->second[i]];
        rs2_video_stream prof;
        prof.type = profile.type;
        prof.index = profile.stream_index;
        prof.uid = profile.uid;
        prof.width = profile.width;
        prof.height = profile.height;
        prof.fps = profile.framerate;
        // TODO - bpp needed?
        prof.fmt = profile.format;
        // TODO - add intrinsics
        fn( prof, profile.default_profile );
    }

    return profiles->second.size();
}


size_t
dds_device::foreach_motion_profile( std::string group_name,
                                    std::function< void( const rs2_motion_stream & profile, bool def_prof ) > fn ) const
{
    auto const& profiles = _impl->_motion_profile_indexes_in_profile_group.find( group_name );

    if (profiles == _impl->_motion_profile_indexes_in_profile_group.end())
    {
        return 0;  // Not motion profiles for this group, nothing to do
    }

    for (size_t i = 0; i < profiles->second.size(); ++i)
    {
        auto const& profile = _impl->_motion_profiles[profiles->second[i]];
        rs2_motion_stream prof;
        prof.type = profile.type;
        prof.index = profile.stream_index;
        prof.uid = profile.uid;
        prof.fps = profile.framerate;
        prof.fmt = profile.format;
        // TODO - add intrinsics
        fn( prof, profile.default_profile );
    }

    return profiles->second.size();
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
