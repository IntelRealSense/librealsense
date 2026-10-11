// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#pragma once

#include <src/uvc-sensor.h>
#include <librealsense2/h/rs_composite_option.h>

#include <memory>
#include <cstdint>
#include <string>


namespace librealsense {

// Generic HKR/D5X5 composite-option embedded filter: registers ONE composite_xu_option under
// `option_id`, in this filter's OWN options container (via `Base`), NOT directly on
// d500_depth_sensor. Every per-feature difference is a constructor argument, not a subclass.
template< class Base, rs2_embedded_filter_type Type >
class composite_embedded_filter : public Base
{
public:
    composite_embedded_filter( std::weak_ptr< uvc_sensor > raw_depth_ep,
                                uint8_t ctrl_id,
                                uint32_t wire_size,
                                rs2_composite_option_id option_id,
                                std::string description );

    rs2_embedded_filter_type get_type() const override { return Type; }
};

}  // namespace librealsense
