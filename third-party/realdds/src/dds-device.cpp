// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-device.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-topic-writer.h>
#include "dds-device-impl.h"


namespace realdds {


dds_device::dds_device( std::shared_ptr< dds_participant > const & participant, topics::device_info const & info )
    : _impl( std::make_shared< dds_device::impl >( participant, info ) )
{
    LOG_DEBUG( "+device '" << _impl->debug_name() << "' on " << info.topic_root );
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

dds_guid const & dds_device::server_guid() const
{
    return _impl->_server_guid;
}

dds_guid const & dds_device::guid() const
{
    return _impl->guid();
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

rsutils::subscription dds_device::on_metadata_available( on_metadata_available_callback && cb )
{
    return _impl->on_metadata_available( std::move( cb ) );
}

rsutils::subscription dds_device::on_device_log( on_device_log_callback && cb )
{
    return _impl->on_device_log( std::move( cb ) );
}

rsutils::subscription dds_device::on_notification( on_notification_callback && cb )
{
    return _impl->on_notification( std::move( cb ) );
}


}  // namespace realdds
