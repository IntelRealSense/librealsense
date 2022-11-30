// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include <third-party/json_fwd.hpp>

#include <string>


namespace realdds {


struct dds_option_range
{
    float min;
    float max;
    float step;
    float default_value;
};

class dds_option
{
    std::string _name;
    float _value;
    dds_option_range _range;
    std::string _description;

    dds_option( nlohmann::json const & j );

public:
    dds_option() = default;

    std::string get_name() const { return _name; }
    void set_name( std::string description ) { _name = description; }

    float get_value() const { return _value; }
    void set_value(float val) { _value = val; }

    dds_option_range get_range() const { return _range; }
    void set_range( dds_option_range range ) { _range = range; }

    std::string get_description() const { return _description; }
    void set_description( std::string description ) { _description = description; }

    nlohmann::json to_json() const;
    static std::shared_ptr< dds_option > from_json( nlohmann::json const & j );
};

typedef std::vector< std::shared_ptr< dds_option > > dds_options;

}  // namespace realdds
