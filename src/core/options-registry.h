// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <librealsense2/h/rs_option.h>
#include <string>


namespace librealsense {


// This registry is used to register options that have custom names. I.e., names that do not have values in
// librealsense's built-in rs2_option enumeration.
// 
// If you need an option "X", it must first be registered. The name and the rs2_option value for it will then be
// associated.
// 
// Such options need unique identifiers that are obviously not in rs2_option and yet must seem like they are. Since
// rs2_option is intergral in nature, we use negative values to identify registered options.
//
class options_registry
{
public:
    // Register a custom option. Option names are unique and case-sensitive. By default, will throw if the option
    // already exists.
    static rs2_option register_option_by_name( std::string const & option_name, bool ok_if_there = false );

    // True if register_option_by_name() was called (i.e., a custom option)
    static bool is_option_registered( rs2_option );

    // Map from option-name to option - works for both custom and built-in options
    static rs2_option find_option_by_name( std::string const & option_name );

    // Returns the name of the option - works only if option is custom, otherwise returns the empty string
    static std::string const & get_registered_option_name( rs2_option );
};


}  // namespace librealsense
