//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "l535-preset-option.h"
#include "l500-private.h"
#include "l500-depth.h"

using librealsense::ivcam2::l535::preset_option;

preset_option::preset_option( const librealsense::option_range & range, std::string description )
    : float_option_with_description< rs2_l500_visual_preset >( range, description )
{
}

void preset_option::set( float value )
{
    if (static_cast<rs2_l500_visual_preset>(int(value)) != RS2_L500_VISUAL_PRESET_CUSTOM)
        throw invalid_value_exception(to_string() <<
            static_cast<rs2_l500_visual_preset>(int(value)) << "not supported!");

    super::set( value );
}

void preset_option::set_value( float value )
{
    super::set( value );
}

        