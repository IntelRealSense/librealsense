// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-option.h>
#include <realdds/dds-exceptions.h>

#include <rsutils/json.h>
using nlohmann::json;


namespace realdds {


dds_option::dds_option( const std::string & name )
    : _name( name )
{
}


dds_option::dds_option( nlohmann::json const & j )
{
    int index = 0;

    _name                = rsutils::json::get< std::string >( j, index++ );
    _value               = rsutils::json::get< float >( j, index++ );
    _range.min           = rsutils::json::get< float >( j, index++ );
    _range.max           = rsutils::json::get< float >( j, index++ );
    _range.step          = rsutils::json::get< float >( j, index++ );
    _range.default_value = rsutils::json::get< float >( j, index++ );
    _description         = rsutils::json::get< std::string >( j, index++ );

    if( index != j.size() )
        DDS_THROW( runtime_error, "expected end of json at index " + std::to_string( index ) );
}


void dds_option::init_stream( std::shared_ptr< dds_stream_base > const & stream )
{
    if( _stream.lock() )
        DDS_THROW( runtime_error, "option '" + get_name() + "' already has a stream" );
    if( ! stream )
        DDS_THROW( runtime_error, "null stream" );
    _stream = stream;
}


/* static  */ std::shared_ptr< dds_option > dds_option::from_json( nlohmann::json const & j )
{
    return std::shared_ptr< dds_option >( new dds_option( j ) );
}


nlohmann::json dds_option::to_json() const
{
    return json::array( { _name, _value, _range.min, _range.max, _range.step, _range.default_value, _description } );
}

}  // namespace realdds
