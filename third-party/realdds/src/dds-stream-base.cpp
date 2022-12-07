// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-stream-base.h>
#include <realdds/dds-utilities.h>


namespace realdds {


dds_stream_base::dds_stream_base( std::string const & name,
                                  std::string const & sensor_name )
    : _name( name )
    , _sensor_name( sensor_name )
{
}


void dds_stream_base::init_profiles_common( dds_stream_profiles const & profiles, int default_profile_index )
{
    if( !_profiles.empty() )
        DDS_THROW( runtime_error, "stream '" + _name + "' profiles are already initialized" );
    if( profiles.empty() )
        DDS_THROW( runtime_error, "at least one profile is required to initialize stream '" + _name + "'" );

    _default_profile_index = default_profile_index;
    if( _default_profile_index < 0 || _default_profile_index >= profiles.size() )
        DDS_THROW( runtime_error,
            "invalid default profile index (" + std::to_string( _default_profile_index ) + " for "
            + std::to_string( profiles.size() ) + " stream profiles" );

    auto self = weak_from_this();
    for( auto const & profile : profiles )
    {
        check_profile( profile );
        profile->init_stream( self );
    }
}


void dds_stream_base::init_profiles( dds_stream_profiles const & profiles, int default_profile_index )
{
    init_profiles_common( profiles, default_profile_index );
    _profiles = profiles;
}


void dds_stream_base::init_profiles( dds_stream_profiles && profiles, int default_profile_index )
{
    init_profiles_common( profiles, default_profile_index );
    _profiles = std::move( profiles );
}


void dds_stream_base::init_options( dds_options const & options )
{
    if( !_options.empty() )
        DDS_THROW( runtime_error, "stream '" + _name + "' options are already initialized" );

    _options = options;
}


void dds_stream_base::init_options( dds_options && options )
{
    if( !_options.empty() )
        DDS_THROW( runtime_error, "stream '" + _name + "' options are already initialized" );

    _options = std::move( options );
}


void dds_stream_base::check_profile( std::shared_ptr< dds_stream_profile > const & profile ) const
{
    if( ! profile )
        DDS_THROW( runtime_error, "null profile passed in" );
}


}  // namespace realdds
