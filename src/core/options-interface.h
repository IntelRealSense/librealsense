// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "option-interface.h"
#include "extension.h"

#include <librealsense2/h/rs_option.h>
#include <vector>
#include <string>


namespace librealsense {


class options_interface : public recordable< options_interface >
{
public:
    virtual option & get_option( rs2_option id ) = 0;
    virtual const option & get_option( rs2_option id ) const = 0;
    virtual bool supports_option( rs2_option id ) const = 0;
    virtual std::vector< rs2_option > get_supported_options() const = 0;
    virtual std::string const & get_option_name( rs2_option ) const = 0;
    virtual ~options_interface() = default;
};

MAP_EXTENSION( RS2_EXTENSION_OPTIONS, librealsense::options_interface );


}  // namespace librealsense
