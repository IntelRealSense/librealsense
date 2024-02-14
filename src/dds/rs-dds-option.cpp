// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "rs-dds-option.h"

#include <realdds/dds-option.h>


namespace librealsense {


static option_range range_from_realdds( std::shared_ptr< realdds::dds_option > const & dds_opt )
{
    if( dds_opt->get_minimum_value().is_number() && dds_opt->get_maximum_value().is_number()
        && dds_opt->get_stepping().is_number() )
    {
        return { dds_opt->get_minimum_value(),
                 dds_opt->get_maximum_value(),
                 dds_opt->get_stepping(),
                 dds_opt->get_default_value() };
    }
    return { 0, 0, 0, 0 };
}


rs_dds_option::rs_dds_option( const std::shared_ptr< realdds::dds_option > & dds_opt,
                              set_option_callback set_opt_cb,
                              query_option_callback query_opt_cb )
    : option_base( range_from_realdds( dds_opt ) )
    , _dds_opt( dds_opt )
    , _set_opt_cb( set_opt_cb )
    , _query_opt_cb( query_opt_cb )
{
}


void rs_dds_option::set( float value )
{
    if( ! _set_opt_cb )
        throw std::runtime_error( "Set option callback is not set for option " + _dds_opt->get_name() );

    _set_opt_cb( _dds_opt->get_name(), value );
}


float rs_dds_option::query() const
{
    if( ! _query_opt_cb )
        throw std::runtime_error( "Query option callback is not set for option " + _dds_opt->get_name() );

    return _query_opt_cb( _dds_opt->get_name() );
}


float rs_dds_option::get_last_known_value() const
{
    return _dds_opt->get_value();
}


bool rs_dds_option::is_read_only() const
{
    return _dds_opt->is_read_only();
}


bool rs_dds_option::is_enabled() const
{
    return ! _dds_opt->get_value().is_null();
}


const char * rs_dds_option::get_description() const
{
    return _dds_opt->get_description().c_str();
}


}  // namespace librealsense
