// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "dds-device.h"
#include "dds-device-impl.h"

namespace librealsense {
namespace dds {


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


size_t dds_device::num_of_sensors() const
{
    return _impl->_num_of_sensors;
}


size_t dds_device::foreach_sensor( std::function< void( const std::string & name ) > fn ) const
{
    for( auto sensor : _impl->_sensor_index_to_name )
    {
        fn( sensor.second );
    }

    return _impl->_num_of_sensors;
}


size_t
dds_device::foreach_video_profile( size_t sensor_index,
                                   std::function< void( const rs2_video_stream & profile, bool def_prof ) > fn ) const
{
    auto const & msg = _impl->_sensor_to_video_profiles.find( int( sensor_index ) );

    if( msg == _impl->_sensor_to_video_profiles.end() )
    {
        return 0;  // Not a video sensor, nothing to do
    }

    for( size_t i = 0; i < msg->second.num_of_profiles; ++i )
    {
        auto const & profile = msg->second.profiles[i];
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

    return msg->second.num_of_profiles;
}


size_t
dds_device::foreach_motion_profile( size_t sensor_index,
                                    std::function< void( const rs2_motion_stream & profile, bool def_prof ) > fn ) const
{
    auto const & msg = _impl->_sensor_to_motion_profiles.find( int( sensor_index ) );

    if( msg == _impl->_sensor_to_motion_profiles.end() )
    {
        return 0;  // Not a motion sensor, nothing to do
    }

    for( size_t i = 0; i < msg->second.num_of_profiles; ++i )
    {
        auto const & profile = msg->second.profiles[i];
        rs2_motion_stream prof;
        prof.type = profile.type;
        prof.index = profile.stream_index;
        prof.uid = profile.uid;
        prof.fps = profile.framerate;
        prof.fmt = profile.format;
        // TODO - add intrinsics
        fn( prof, profile.default_profile );
    }

    return msg->second.num_of_profiles;
}


}  // namespace dds
}  // namespace librealsense
