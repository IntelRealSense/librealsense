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
    _name                = j["name"];
    _value               = j["value"];
    _range.min           = j["range-min"];
    _range.max           = j["range-max"];
    _range.step          = j["range-step"];
    _range.default_value = j["range-default"];
    _description         = j["description"];

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
