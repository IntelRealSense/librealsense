// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-stream.h>
#include "dds-stream-impl.h"

#include <stdexcept>

namespace realdds {


dds_video_stream::dds_video_stream( const std::string & group_name )
    : _impl( std::make_shared< dds_video_stream::impl >( group_name ) )
{
}

const std::string & dds_video_stream::get_group_name() const
{
    return _impl->_group_name;
}

void dds_video_stream::add_profile( dds_video_stream_profile && prof, bool default_profile )
{
    _impl->_profiles.push_back( std::make_pair( std::move( prof ), default_profile ) );
}

size_t dds_video_stream::foreach_profile( std::function< void( const dds_stream_profile & prof, bool def_prof ) > fn ) const
{
    for ( auto profile : _impl->_profiles )
    {
        fn( profile.first, profile.second );
    }

    return _impl->_profiles.size();
}

dds_motion_stream::dds_motion_stream( const std::string & group_name )
    : _impl( std::make_shared< dds_motion_stream::impl >( group_name ) )
{
}

const std::string & dds_motion_stream::get_group_name() const
{
    return _impl->_group_name;
}

void dds_motion_stream::add_profile( dds_motion_stream_profile && prof, bool default_profile )
{
    _impl->_profiles.push_back( std::make_pair( std::move( prof ), default_profile ) );
}

size_t dds_motion_stream::foreach_profile( std::function< void( const dds_stream_profile & prof, bool def_prof ) > fn ) const
{
    for ( auto profile : _impl->_profiles )
    {
        fn( profile.first, profile.second );
    }

    return _impl->_profiles.size();
}

}  // namespace realdds
