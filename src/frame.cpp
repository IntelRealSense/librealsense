// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.
#include "frame.h"
#include "archive.h"
#include "metadata-parser.h"
#include "environment.h"

namespace librealsense {

std::ostream & operator<<( std::ostream & os, frame_header const & header )
{
    os << "#" << header.frame_number;
    os << " @" << ( std::string )( to_string() << std::fixed << std::setprecision( 2 ) << (double)header.timestamp );
    if( header.timestamp_domain != RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK )
        os << "/" << rs2_timestamp_domain_to_string( header.timestamp_domain );
    return os;
}


frame::frame( frame && r )
    : ref_count( r.ref_count.exchange( 0 ) )
    , owner( r.owner )
    , on_release()
    , _kept( r._kept.exchange( false ) )
{
    *this = std::move( r );
    if( owner )
        metadata_parsers = owner->get_md_parsers();
    if( r.metadata_parsers )
        metadata_parsers = std::move( r.metadata_parsers );
}

frame & frame::operator=( frame && r )
{
    data = move( r.data );
    owner = r.owner;
    ref_count = r.ref_count.exchange( 0 );
    _kept = r._kept.exchange( false );
    on_release = std::move( r.on_release );
    additional_data = std::move( r.additional_data );
    r.owner.reset();
    if( owner )
        metadata_parsers = owner->get_md_parsers();
    if( r.metadata_parsers )
        metadata_parsers = std::move( r.metadata_parsers );
    return *this;
}
archive_interface * frame::get_owner() const
{
    return owner.get();
}

std::shared_ptr< sensor_interface > frame::get_sensor() const
{
    auto res = sensor.lock();
    if( ! res )
    {
        auto archive = get_owner();
        if( archive )
            return archive->get_sensor();
    }
    return res;
}
void frame::set_sensor( std::shared_ptr< sensor_interface > s )
{
    sensor = s;
}

void frame::release()
{
    if( ref_count.fetch_sub( 1 ) == 1 && owner )
    {
        unpublish();
        on_release();
        owner->unpublish_frame( this );
    }
}

void frame::keep()
{
    if( ! _kept.exchange( true ))
    {
        if (owner)
            owner->keep_frame( this );
    }
}

frame_interface * frame::publish( std::shared_ptr< archive_interface > new_owner )
{
    owner = new_owner;
    _kept = false;
    return owner->publish_frame( this );
}

rs2_metadata_type frame::get_frame_metadata( const rs2_frame_metadata_value & frame_metadata ) const
{
    if( ! metadata_parsers )
        throw invalid_value_exception( to_string() << "metadata not available for "
                                                   << get_string( get_stream()->get_stream_type() )
                                                   << " stream" );

    auto parsers = metadata_parsers->equal_range( frame_metadata );
    if( parsers.first
        == metadata_parsers
               ->end() )  // Possible user error - md attribute is not supported by this frame type
        throw invalid_value_exception(
            to_string() << get_string( frame_metadata ) << " attribute is not applicable for "
                        << get_string( get_stream()->get_stream_type() ) << " stream " );

    rs2_metadata_type result = -1;
    bool value_retrieved = false;
    std::string exc_str;
    for( auto it = parsers.first; it != parsers.second; ++it )
    {
        try
        {
            result = it->second->get( *this );
            value_retrieved = true;
            break;
        }
        catch( invalid_value_exception & e )
        {
            exc_str = e.what();
        }
    }
    if( ! value_retrieved )
        throw invalid_value_exception( exc_str );

    return result;
}

bool frame::supports_frame_metadata( const rs2_frame_metadata_value & frame_metadata ) const
{
    // verify preconditions
    if( ! metadata_parsers )
        return false;  // No parsers are available or no metadata was attached

    bool ret = false;
    auto found = metadata_parsers->equal_range( frame_metadata );
    if( found.first == metadata_parsers->end() )
        return false;

    for( auto it = found.first; it != found.second; ++it )
        if( it->second->supports( *this ) )
        {
            ret = true;
            break;
        }

    return ret;
}

int frame::get_frame_data_size() const
{
    return (int)data.size();
}

const byte * frame::get_frame_data() const
{
    const byte * frame_data = data.data();

    if( on_release.get_data() )
    {
        frame_data = static_cast< const byte * >( on_release.get_data() );
    }

    return frame_data;
}

rs2_timestamp_domain frame::get_frame_timestamp_domain() const
{
    return additional_data.timestamp_domain;
}

rs2_time_t frame::get_frame_timestamp() const
{
    return additional_data.timestamp;
}

unsigned long long frame::get_frame_number() const
{
    return additional_data.frame_number;
}

rs2_time_t frame::get_frame_system_time() const
{
    return additional_data.system_time;
}

float depth_frame::get_distance( int x, int y ) const
{
    // If this frame does not itself contain Z16 depth data,
    // fall back to the original frame it was created from
    if( _original && get_stream()->get_format() != RS2_FORMAT_Z16 )
        return ( (depth_frame *)_original.frame )->get_distance( x, y );

    uint64_t pixel = 0;
    switch( get_bpp() / 8 )  // bits per pixel
    {
    case 1:
        pixel = get_frame_data()[y * get_width() + x];
        break;
    case 2:
        pixel = reinterpret_cast< const uint16_t * >( get_frame_data() )[y * get_width() + x];
        break;
    case 4:
        pixel = reinterpret_cast< const uint32_t * >( get_frame_data() )[y * get_width() + x];
        break;
    case 8:
        pixel = reinterpret_cast< const uint64_t * >( get_frame_data() )[y * get_width() + x];
        break;
    default:
        throw std::runtime_error( to_string() << "Unrecognized depth format "
                                              << int( get_bpp() / 8 ) << " bytes per pixel" );
    }

    return pixel * get_units();
}

}  // namespace librealsense
