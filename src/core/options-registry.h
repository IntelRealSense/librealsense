// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <librealsense2/h/rs_option.h>
#include <string>


namespace librealsense {


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
