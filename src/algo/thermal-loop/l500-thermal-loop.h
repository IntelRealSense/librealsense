// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>
#include "../../types.h"

namespace librealsense {
namespace algo {
namespace thermal_loop {
namespace l500 {

#pragma pack (push,1)

// this struct based on RGB_Thermal_Info_CalibInfo table 
struct rgb_thermal_calib_info
{
    static const int resolution = 29;

    struct table_meta_data
    {
        float min_temp;
        float max_temp;
        float reference_temp; // not used
        float valid;  // not used
    };

    struct table_data
    {
        float scale;
        float p[3];  // parameters which effects offset that are not in use
    };

    table_meta_data md;
    std::vector< table_data > vals;
};
#pragma pack(pop)

rgb_thermal_calib_info parse_thermal_table( std::vector< byte > data );

double get_rgb_current_thermal_scale( const rgb_thermal_calib_info & table, double hum_temp );

std::pair< double, double > correct_thermal_scale( std::pair< double, double > in_calib,
                                                   double scale );

}  // namespace l500
}  // namespace thermal_loop
}  // namespace algo
}  // namespace librealsense
