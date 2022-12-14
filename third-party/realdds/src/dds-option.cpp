// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-option.h>
#include <realdds/dds-exceptions.h>

#include <librealsense2/utilities/json.h>
using nlohmann::json;


namespace realdds {


dds_option::dds_option( const std::string & name, const std::string & owner_name )
    : _name( name )
    , _owner_name( owner_name )
{
}


dds_option::dds_option( nlohmann::json const & j, const std::string & owner_name )
{
    _name                = utilities::json::get< std::string >( j, "name" );
    _value               = utilities::json::get< float >( j, "value" );
    _range.min           = utilities::json::get< float >( j, "range-min" );
    _range.max           = utilities::json::get< float >( j, "range-max" );
    _range.step          = utilities::json::get< float >( j, "range-step" );
    _range.default_value = utilities::json::get< float >( j, "range-default" );
    _description         = utilities::json::get< std::string >( j, "description" );

    _owner_name = owner_name;
}


/* static  */ std::shared_ptr< dds_option > dds_option::from_json(nlohmann::json const & j, const std::string & owner_name )
{
    return std::shared_ptr< dds_option >( new dds_option( j, owner_name ) );
}


nlohmann::json dds_option::to_json() const
{
    return json( {
        { "name", _name },
        { "value", _value },
        { "range-min", _range.min },
        { "range-max", _range.max },
        { "range-step", _range.step },
        { "range-default", _range.default_value },
        { "description", _description }
        } );
}

}  // namespace realdds
