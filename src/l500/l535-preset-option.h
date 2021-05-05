// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once
#include "hw-monitor.h"
#include "l500-device.h"

namespace librealsense {
namespace ivcam2 {
namespace l535 {

    class preset_option : public float_option_with_description< rs2_l500_visual_preset >
    {
        typedef float_option_with_description< rs2_l500_visual_preset > super;
    public:
        preset_option( const option_range& range, std::string description );
        void set( float value ) override;
        void set_value( float value );
    };

}  // namespace l535
}  // namespace ivcam2
}  // namespace librealsense
