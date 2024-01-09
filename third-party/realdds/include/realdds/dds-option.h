// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.
#pragma once

#include <rsutils/json-fwd.h>

#include <string>
#include <vector>
#include <memory>


namespace realdds {


struct dds_option_range
{
    float min = 0.0f;
    float max = 0.0f;
    float step = 0.0f;
    float default_value = 0.0f;
};

class dds_stream_base;

class dds_option
{
    std::string _name;
    float _value = 0.0f;
    dds_option_range _range;
    std::string _description;

    dds_option( rsutils::json const & );

    friend class dds_stream_base;
    std::weak_ptr< dds_stream_base > _stream;
    void init_stream( std::shared_ptr< dds_stream_base > const & );

public:
    dds_option( const std::string & name, dds_option_range const &, std::string const & description );

    const std::string & get_name() const { return _name; }
    std::shared_ptr< dds_stream_base > stream() const { return _stream.lock(); }

    float get_value() const { return _value; }
    void set_value(float val) { _value = val; }

    const dds_option_range & get_range() const { return _range; }
    const std::string & get_description() const { return _description; }

    rsutils::json to_json() const;
    static std::shared_ptr< dds_option > from_json( rsutils::json const & j );
};

typedef std::vector< std::shared_ptr< dds_option > > dds_options;


}  // namespace realdds
