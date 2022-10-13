// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-stream.h>
#include "dds-stream-impl.h"

#include <stdexcept>

namespace realdds {


dds_video_stream::dds_video_stream( int8_t type, const std::string & group_name )
    : _impl( std::make_shared< dds_video_stream::impl >( type, group_name ) )
{
}

int8_t dds_video_stream::get_type() const
{
    return _impl->_type;
}

const std::string & dds_video_stream::get_group_name() const
{
    return _impl->_group_name;
}

void dds_video_stream::add_profile( const dds_stream::profile & prof, bool default_profile )
{
    if ( _impl->_type != prof.type )
    {
        throw std::runtime_error( "profile of different type then stream" );
    }

    _impl->_profiles.push_back( std::make_pair( static_cast< const dds_video_stream::profile & >( prof ), default_profile ) );
}

size_t dds_video_stream::foreach_profile( std::function< void( const dds_stream::profile & prof, bool def_prof ) > fn ) const
{
    for ( auto profile : _impl->_profiles )
    {
        fn( profile.first, profile.second );
    }

    return _impl->_profiles.size();
}

dds_motion_stream::dds_motion_stream( int8_t type, const std::string & group_name )
    : _impl( std::make_shared< dds_motion_stream::impl >( type, group_name ) )
{
}

int8_t dds_motion_stream::get_type() const
{
    return _impl->_type;
}

const std::string & dds_motion_stream::get_group_name() const
{
    return _impl->_group_name;
}

void dds_motion_stream::add_profile( const dds_stream::profile & prof, bool default_profile )
{
    if ( _impl->_type != prof.type )
    {
        throw std::runtime_error( "profile of different type then stream" );
    }

    _impl->_profiles.push_back( std::make_pair( static_cast< const dds_motion_stream::profile & >( prof ), default_profile ) );
}

size_t dds_motion_stream::foreach_profile( std::function< void( const dds_stream::profile & prof, bool def_prof ) > fn ) const
{
    for ( auto profile : _impl->_profiles )
    {
        fn( profile.first, profile.second );
    }

    return _impl->_profiles.size();
}

}  // namespace realdds
