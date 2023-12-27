// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-option.h>
#include <realdds/dds-exceptions.h>

#include <rsutils/json.h>


namespace realdds {


dds_option::dds_option( const std::string & name, dds_option_range const & range, std::string const & description )
    : _name( name )
    , _range( range )
    , _value( range.default_value )
    , _description( description )
{
}


dds_option::dds_option( rsutils::json const & j )
{
    int index = 0;

    _name                = j[index++].string_ref();
    _value               = j[index++].get< float >();
    _range.min           = j[index++].get< float >();
    _range.max           = j[index++].get< float >();
    _range.step          = j[index++].get< float >();
    _range.default_value = j[index++].get< float >();
    _description         = j[index++].string_ref();

    if( index != j.size() )
        DDS_THROW( runtime_error, "expected end of json at index " << index );
}


void dds_option::init_stream( std::shared_ptr< dds_stream_base > const & stream )
{
    if( _stream.lock() )
        DDS_THROW( runtime_error, "option '" << get_name() << "' already has a stream" );
    if( ! stream )
        DDS_THROW( runtime_error, "null stream" );
    _stream = stream;
}


/*static*/ std::shared_ptr< dds_option > dds_option::from_json( rsutils::json const & j )
{
    return std::shared_ptr< dds_option >( new dds_option( j ) );
}


rsutils::json dds_option::to_json() const
{
    return rsutils::json::array( { _name, _value, _range.min, _range.max, _range.step, _range.default_value, _description } );
}

}  // namespace realdds
