// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../../../src/core/advanced_mode.h"

ds5_advanced_mode_base::ds5_advanced_mode_base(std::shared_ptr<hw_monitor> hwm, uvc_sensor& depth_sensor)
    : _hw_monitor(hwm),
      _depth_sensor(depth_sensor)
{
            _depth_sensor.register_option(RS2_OPTION_ADVANCED_MODE_PRESET,
                                          std::make_shared<advanced_mode_preset_option>(*this,
                                                                                        _depth_sensor,
                                                                                        option_range{0,
                                                                                                     RS2_ADVANCED_MODE_PRESET_COUNT - 1,
                                                                                                     1,
                                                                                                     0},
                                                                                        _description_per_value));
}
