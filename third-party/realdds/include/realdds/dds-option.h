// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include <nlohmann/json_fwd.hpp>

#include <string>
#include <vector>


namespace realdds {

struct dds_option_range
{
    float min = 0.0f;
    float max = 0.0f;
    float step = 0.0f;
    float default_value = 0.0f;
};

class dds_option
{
    std::string _name;
    float _value = 0.0f;
    dds_option_range _range;
    std::string _description;

    std::string _owner_name; //To identify device or stream having this option

    dds_option( nlohmann::json const & j, const std::string & owner_name );

public:
    dds_option( const std::string & name, const std::string & owner_name );

    const std::string & get_name() const { return _name; }
    const std::string & owner_name() const { return _owner_name; }

    float get_value() const { return _value; }
    void set_value(float val) { _value = val; }

    const dds_option_range & get_range() const { return _range; }
    void set_range( const dds_option_range & range ) { _range = range; }

    const std::string & get_description() const { return _description; }
    void set_description( const std::string & description ) { _description = description; }


    nlohmann::json to_json() const;
    static std::shared_ptr< dds_option > from_json( nlohmann::json const & j, const std::string & stream_name );
};

typedef std::vector< std::shared_ptr< dds_option > > dds_options;

}  // namespace realdds
