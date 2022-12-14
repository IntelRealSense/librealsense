// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>
#include "../../types.h"
#include "thermal-calibration-table-interface.h"

namespace librealsense {
namespace algo {
namespace thermal_loop {
namespace l500 {


// RGB_Thermal_Info_CalibInfo table
// -----------------------------------
//
// The table contains equally spaced bins between min & max temperature.
//
// Each bin has a set of 4 transformation parameters. The transformation maps a point in the RGB
// image in a given temperature to its expected location in the temperature in which the RGB module
// was calibrated.
//
// Reference at:
// https://rsconf.intel.com/display/L500/0x317+RGB+Thermal+Table
//
#pragma pack( push, 1 )
class thermal_calibration_table : public thermal_calibration_table_interface
{
public:
    static const int id;
    
    // The header as it's written raw
    struct thermal_table_header
    {
        float min_temp;
        float max_temp;
        float reference_temp;  // Reference calibration temperature (humidity sensor)
        float valid;           // Valid table (Created in ATC && Updated in ACC)
    };

    // Each bin data, as it's written in the actual raw table
    struct thermal_bin
    {
        float scale;
        float sheer; // parameters which affect offset that are not in use
        float tx;
        float ty;
    };

    // Number of bins *between* min and max:
    //     bin - size = ( max - min ) / ( resolution + 1 )
    // In the raw calibration table, 29 is implicitly used.
    size_t _resolution; 
    
    thermal_table_header _header;
    std::vector< thermal_bin > bins;

    thermal_calibration_table();
    thermal_calibration_table( const std::vector< byte > & data, int resolution = 29 );

    double get_thermal_scale( double hum_temp ) const override;

    std::vector< byte > build_raw_data() const override;

    bool is_valid() const override { return _header.valid != 0.f; }
};
#pragma pack( pop )

bool operator==( const thermal_calibration_table & lhs, const thermal_calibration_table & rhs );

}  // namespace l500
}  // namespace thermal_loop
}  // namespace algo
}  // namespace librealsense
