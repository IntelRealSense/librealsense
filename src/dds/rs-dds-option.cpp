// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "rs-dds-option.h"

#include <realdds/dds-option.h>
#include <rsutils/json.h>

using rsutils::json;


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
    if( std::dynamic_pointer_cast< realdds::dds_boolean_option >( dds_opt ) )
    {
        return { 0, 1, 1, bool( dds_opt->get_default_value() ) ? 1.f : 0.f };
    }
    if( auto e = std::dynamic_pointer_cast< realdds::dds_enum_option >( dds_opt ) )
    {
        return { 0, float( e->get_choices().size() - 1 ), 1, (float)e->get_value_index( e->get_default_value() ) };
    }
    return { 0, 0, 0, 0 };
}


static rs2_option_type rs_type_from_dds_option( std::shared_ptr< realdds::dds_option > const & dds_opt )
{
    if( std::dynamic_pointer_cast< realdds::dds_float_option >( dds_opt ) )
        return RS2_OPTION_TYPE_FLOAT;
    if( std::dynamic_pointer_cast< realdds::dds_string_option >( dds_opt ) )
        return RS2_OPTION_TYPE_STRING;
    if( std::dynamic_pointer_cast< realdds::dds_boolean_option >( dds_opt ) )
        return RS2_OPTION_TYPE_BOOLEAN;
    if( std::dynamic_pointer_cast< realdds::dds_integer_option >( dds_opt ) )
        return RS2_OPTION_TYPE_INTEGER;
    throw not_implemented_exception( "unknown DDS option type" );
}


rs_dds_option::rs_dds_option( const std::shared_ptr< realdds::dds_option > & dds_opt,
                              set_option_callback set_opt_cb,
                              query_option_callback query_opt_cb )
    : option_base( range_from_realdds( dds_opt ) )
    , _dds_opt( dds_opt )
    , _rs_type( rs_type_from_dds_option( dds_opt ) )
    , _set_opt_cb( set_opt_cb )
    , _query_opt_cb( query_opt_cb )
{
}


void rs_dds_option::set( float value )
{
    if( ! _set_opt_cb )
        throw std::runtime_error( "Set option callback is not set for option " + _dds_opt->get_name() );

    _set_opt_cb( value );
}


float rs_dds_option::query() const
{
    if( ! _query_opt_cb )
        throw std::runtime_error( "Query option callback is not set for option " + _dds_opt->get_name() );

    auto const value = _query_opt_cb();
    try
    {
        return value;  // try to convert to float
    }
    catch( std::exception const & )
    {
        throw invalid_value_exception( rsutils::string::from() << "option '" << _dds_opt->get_name() << "' value (" << value << ") is not a float" );
    }
}


json rs_dds_option::get_value() const noexcept
{
    return _dds_opt->get_value();
}


void rs_dds_option::set_value( json value )
{
    if( ! _set_opt_cb )
        throw std::runtime_error( "Set option callback is not set for option " + _dds_opt->get_name() );

    _set_opt_cb( std::move( value ) );
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


const char * rs_dds_option::get_value_description( float v ) const
{
    auto e = std::dynamic_pointer_cast< realdds::dds_enum_option >( _dds_opt );
    if( ! e )
        return nullptr;
    if( v < 0.f )
        return nullptr;
    auto & choices = e->get_choices();
    auto i = size_t( v + 0.005 );  // round to nearest integer value
    if( fabs( v - i ) > 0.01f )    // verify we're approx equal to it, since float == is not reliable
        return nullptr;
    if( i >= choices.size() )
        return nullptr;
    return choices[i].c_str();
}


}  // namespace librealsense
