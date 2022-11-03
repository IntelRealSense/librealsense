// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-device.h>
#include "dds-device-impl.h"

#include <realdds/topics/flexible/flexible-msg.h>
#include <realdds/dds-exceptions.h>

#include <third-party/json.hpp>

namespace realdds {


namespace {

// The map from guid to device is global
typedef std::map< dds_guid, std::weak_ptr< dds_device > > guid_to_device_map;
guid_to_device_map guid_to_device;
std::mutex devices_mutex;

}  // namespace


std::shared_ptr< dds_device > dds_device::find( dds_guid const & guid )
{
    std::lock_guard< std::mutex > lock( devices_mutex );
    return find_internal( guid );
}

std::shared_ptr< dds_device > dds_device::find_internal( dds_guid const & guid)
{
    //Assumes devices_mutex is locked outside this function to protect access to guid_to_device

    std::shared_ptr< dds_device > dev;

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
    // Locking before find_internal() to avoid a device created between search and creation
    std::lock_guard< std::mutex > lock( devices_mutex );

    std::shared_ptr< dds_device > dev = find_internal( guid );

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


std::shared_ptr< dds_participant > const& dds_device::participant() const
{
    return _impl->_participant;
}


topics::device_info const & dds_device::device_info() const
{
    return _impl->_info;
}

dds_guid const & dds_device::guid() const
{
    return _impl->_guid;
}

size_t dds_device::number_of_streams() const
{
    return _impl->_streams.size();
}

size_t dds_device::foreach_stream( std::function< void( std::shared_ptr< dds_stream > stream ) > fn ) const
{
    for ( auto const & stream : _impl->_streams )
    {
        fn( stream.second );
    }

    return _impl->_streams.size();
}

void dds_device::open( const dds_stream_profiles & profiles )
{
    using nlohmann::json;
    auto stream_profiles = json::array();
    for( auto & profile : profiles )
    {
        auto stream = profile->stream();
        if( ! stream )
            DDS_THROW( runtime_error, "profile (" + profile->to_string() + ") is not part of any stream" );
        stream_profiles += json::array( { stream->name(), profile->to_json() } );
    }
    json j = {
        { "id", "open-streams" },
        { "stream-profiles", stream_profiles },
    };
    _impl->write_control_message( j );
}

void dds_device::close( dds_streams const & streams )
{
    using nlohmann::json;
    auto stream_names = json::array();
    for( auto & stream : streams )
    {
        if( ! stream )
            DDS_THROW( runtime_error, "null stream passed in" );
        stream_names += stream->name();
    }
    json j = {
        { "id", "close-streams" },
        { "stream-names", stream_names },
    };
    _impl->write_control_message( j );
}


}  // namespace realdds
