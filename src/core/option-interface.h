// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include "extension.h"
#include <src/basics.h>
#include <librealsense2/h/rs_option.h>
#include <rsutils/json-fwd.h>


namespace librealsense {


struct LRS_EXTENSION_API option_range
{
    float min;
    float max;
    float step;
    float def;
};

class LRS_EXTENSION_API option : public recordable< option >
{
public:
    virtual void set( float value ) = 0;
    virtual float query() const = 0;

    // Return the value currently assigned to the option
    // By default, this is the result of query()
    virtual rsutils::json get_value() const noexcept;

    // Return the type of the option value
    virtual rs2_option_type get_value_type() const noexcept;

    // Set the value, or throw an error
    // By default, this does a set() with a float
    virtual void set_value( rsutils::json );

    virtual option_range get_range() const = 0;
    virtual bool is_enabled() const = 0;
    virtual bool is_read_only() const { return false; }
    virtual const char * get_description() const = 0;
    virtual const char * get_value_description( float ) const { return nullptr; }
    
    // recordable< option >
    virtual void create_snapshot( std::shared_ptr< option > & snapshot ) const override;

    virtual ~option() = default;
};


}  // namespace librealsense
