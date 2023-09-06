// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-device.h>
#include <realdds/dds-participant.h>
#include "dds-device-impl.h"


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

std::shared_ptr< dds_device > dds_device::find_internal( dds_guid const & guid )
{
    // Assumes devices_mutex is locked outside this function to protect access to guid_to_device

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
        LOG_DEBUG( "+device '" << info.debug_name() << "' (" << participant->print( guid ) << ") on " << info.topic_root );
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

bool dds_device::is_ready() const
{
    return _impl->is_ready();
}

void dds_device::wait_until_ready( size_t timeout_ns )
{
    _impl->wait_until_ready( timeout_ns );
}

std::shared_ptr< dds_participant > const& dds_device::participant() const
{
    return _impl->_participant;
}

std::shared_ptr< dds_subscriber > const & dds_device::subscriber() const
{
    return _impl->_subscriber;
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

size_t dds_device::foreach_option( std::function< void( std::shared_ptr< dds_option > option ) > fn ) const
{
    for( auto const & option : _impl->_options)
    {
        fn( option );
    }

    return _impl->_options.size();
}

void dds_device::open( const dds_stream_profiles & profiles )
{
    _impl->open( profiles );
}

void dds_device::set_option_value( const std::shared_ptr< dds_option > & option, float new_value )
{
    _impl->set_option_value( option, new_value );
}

float dds_device::query_option_value( const std::shared_ptr< dds_option > & option )
{
    return _impl->query_option_value( option );
}

void dds_device::send_control( topics::flexible_msg && msg, nlohmann::json * reply )
{
    _impl->write_control_message( std::move( msg ), reply );
}

bool dds_device::has_extrinsics() const
{
    return ! _impl->_extrinsics_map.empty();
}

std::shared_ptr< extrinsics > dds_device::get_extrinsics( std::string const & from, std::string const & to ) const
{
    auto iter = _impl->_extrinsics_map.find( std::make_pair( from, to ) );
    if( iter != _impl->_extrinsics_map.end() )
        return iter->second;

    std::shared_ptr< extrinsics > empty;
    return empty;
}

bool dds_device::supports_metadata() const
{
    return !! _impl->_metadata_reader;
}

void dds_device::on_metadata_available( on_metadata_available_callback cb )
{
    _impl->on_metadata_available( cb );
}

void dds_device::on_device_log( on_device_log_callback cb )
{
    _impl->on_device_log( cb );
}


}  // namespace realdds
