//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "l535-preset-option.h"
#include "l500-private.h"
#include "l500-depth.h"

namespace librealsense
{
    namespace ivcam2
    {
        namespace l535
        {
            l535_preset_option::l535_preset_option(option_range range, std::string description)
                :float_option_with_description< rs2_l500_visual_preset >(range, description)
            {}

            void l535_preset_option::set(float value)
            {
                if (static_cast<rs2_l500_visual_preset>(int(value))
                    == RS2_L500_VISUAL_PRESET_DEFAULT)
                    throw invalid_value_exception(to_string()
                        << "RS2_L500_VISUAL_PRESET_DEFAULT was deprecated!");

                float_option_with_description< rs2_l500_visual_preset >::set(value);
            }

            void l535_preset_option::set_value(float value)
            {
                float_option_with_description< rs2_l500_visual_preset >::set(value);
            }

         } // namespace l535
    } // namespace ivcam2
} // namespace librealsense
