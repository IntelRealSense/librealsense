// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "rs-dds-option.h"

#include <realdds/dds-option.h>


namespace librealsense {


rs_dds_option::rs_dds_option( const std::shared_ptr< realdds::dds_option > & dds_opt,
                              set_option_callback set_opt_cb,
                              query_option_callback query_opt_cb )
    : option_base( { dds_opt->get_range().min,
                     dds_opt->get_range().max,
                     dds_opt->get_range().step,
                     dds_opt->get_range().default_value } )
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


const char * rs_dds_option::get_description() const
{
    return _dds_opt->get_description().c_str();
}


}  // namespace librealsense
