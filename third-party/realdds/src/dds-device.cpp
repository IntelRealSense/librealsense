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

void dds_device::open( const std::vector< dds_video_stream::profile > & profiles )
{
    using namespace topics;

    if ( profiles.size() > device::control::MAX_OPEN_STREAMS )
    {
        throw std::length_error( "Too many streams to open (" + std::to_string( profiles.size() )
            + "), max is " + std::to_string( device::control::MAX_OPEN_STREAMS ) );
    }

    device::control::streams_open_msg open_msg;
    open_msg.message_id = _impl->_control_message_counter++;
    for ( size_t i = 0; i < profiles.size(); ++i )
    {
        open_msg.streams[i].uid = profiles[i].uid;
        open_msg.streams[i].framerate = profiles[i].framerate;
        open_msg.streams[i].format = profiles[i].format;
        open_msg.streams[i].type = profiles[i].type;
        open_msg.streams[i].width = profiles[i].width;
        open_msg.streams[i].height = profiles[i].height;
    }

    raw::device::control raw_msg;
    device::control::construct_raw_message( device::control::msg_type::STREAMS_OPEN,
        open_msg,
        raw_msg );

    if ( _impl->write_control_message( &raw_msg ) )
    {
        LOG_DEBUG( "Sent STREAMS_OPEN message for " << profiles.size() << " streams" );
    }
    else
    {
        LOG_ERROR( "Error writing STREAMS_OPEN message for " << profiles.size() << " streams" );
    }
}

void dds_device::open( const std::vector< dds_motion_stream::profile > & profiles )
{
    using namespace topics;

    if ( profiles.size() > device::control::MAX_OPEN_STREAMS )
    {
        throw std::length_error( "Too many streams to open (" + std::to_string( profiles.size() )
                                 + "), max is " + std::to_string( device::control::MAX_OPEN_STREAMS ) );
    }

    device::control::streams_open_msg open_msg;
    open_msg.message_id = _impl->_control_message_counter++;
    for ( size_t i = 0; i < profiles.size(); ++i )
    {
        open_msg.streams[i].uid       = profiles[i].uid;
        open_msg.streams[i].framerate = profiles[i].framerate;
        open_msg.streams[i].format    = profiles[i].format;
        open_msg.streams[i].type      = profiles[i].type;
        open_msg.streams[i].width     = 0;
        open_msg.streams[i].height    = 0;
    }

    raw::device::control raw_msg;
    device::control::construct_raw_message( device::control::msg_type::STREAMS_OPEN,
                                            open_msg,
                                            raw_msg );

    if ( _impl->write_control_message( &raw_msg ) )
    {
        LOG_DEBUG( "Sent STREAMS_OPEN message for " << profiles.size() << " streams" );
    }
    else
    {
        LOG_ERROR( "Error writing STREAMS_OPEN message for " << profiles.size() << " streams" );
    }
}

void dds_device::close( const std::vector< int16_t >& stream_uids )
{
    using namespace topics;

    if ( stream_uids.size() > device::control::MAX_OPEN_STREAMS )
    {
        throw std::runtime_error( "Too many profiles to close (" + std::to_string( stream_uids.size() )
                                + "), max is " + std::to_string( device::control::MAX_OPEN_STREAMS ) );
    }

    device::control::streams_close_msg close_msg;
    close_msg.message_id = _impl->_control_message_counter++;
    for (size_t i = 0; i < stream_uids.size(); ++i)
    {
        close_msg.stream_uids[i] = stream_uids[i];
    }

    raw::device::control raw_msg;
    device::control::construct_raw_message( device::control::msg_type::STREAMS_CLOSE,
                                            close_msg,
                                            raw_msg );

    if (_impl->write_control_message( &raw_msg ))
    {
        LOG_DEBUG( "Sent STREAMS_CLOSE message for " << stream_uids.size() << " streams" );
    }
    else
    {
        LOG_ERROR( "Error writing STREAMS_CLOSE message for " << stream_uids.size() << " streams" );
    }
}


}  // namespace realdds
