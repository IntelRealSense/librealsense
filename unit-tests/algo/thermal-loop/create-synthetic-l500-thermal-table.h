// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../algo-common.h"
#include <src/algo/thermal-loop/l500-thermal-loop.h>

namespace thermal = librealsense::algo::thermal_loop::l500;

// Create a fictitious table with min=0 and max depending on the table size
thermal::thermal_calibration_table
create_synthetic_table( const int table_size = 29, float min_temp = 0, float max_temp = 75 )
{
    thermal::thermal_calibration_table res;
    res._header.min_temp = min_temp;
    res._header.max_temp = max_temp;
    res._header.reference_temp = 35.f;
    res._header.valid = 1.f;
    res._resolution = table_size;
    res.bins.resize( table_size );

    // [0   - 2.5]  --> 0.5 
    // (2.5 - 5]    --> 1 
    // (5   - 7.5]  --> 1.5 
    // (7.5 - 10]   --> 2 
    // ...
    // (72.5 - 75]  --> 15
    for( auto i = 0; i < table_size; i++ )
    {
        res.bins[i].scale = ( i + 1 ) * 0.5f;
        res.bins[i].sheer = 0;
        res.bins[i].tx = 0;
        res.bins[i].ty = 0;
    }
    return res;
}
