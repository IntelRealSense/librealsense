// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>

namespace librealsense {
namespace algo {
namespace thermal_loop {


struct thermal_calibration_table_interface
{
    virtual ~thermal_calibration_table_interface() {}

    virtual bool is_valid() const = 0;

    virtual double get_thermal_scale( double hum_temp ) const = 0;

    virtual std::vector< byte > build_raw_data() const = 0;
};


}  // namespace thermal_loop
}  // namespace algo
}  // namespace librealsense
