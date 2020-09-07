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

static rgb_thermal_calib_info parse_thermal_table( std::vector< byte > data )
{
    rgb_thermal_calib_info res;
    std::vector< float > float_vac;
    float_vac.assign( (float *)data.data(),
                      (float *)data.data() + ( data.size() / sizeof( float ) ) );

    res.md.min_temp = float_vac[0];
    res.md.max_temp = float_vac[1];

    const int meta_data_size = sizeof( rgb_thermal_calib_info::table_meta_data ) / sizeof( float );
    const int table_data_size = sizeof( rgb_thermal_calib_info::table_data ) / sizeof( float );
    if( ( float_vac.size() - meta_data_size ) / table_data_size
        == rgb_thermal_calib_info::resolution )
    {
        res.md.reference_temp = float_vac[2];
        res.md.valid = float_vac[3];
        res.vals.assign(
            (rgb_thermal_calib_info::table_data *)( float_vac.data() + meta_data_size ),
            (rgb_thermal_calib_info::table_data *)float_vac.data()
                + rgb_thermal_calib_info::resolution );
    }
    else
    {
        throw std::runtime_error( librealsense::to_string() << "file size (" << data.size()
                                                            << ") does not match data size " );
    }

    return res;
}

static double get_rgb_current_thermal_scale( const rgb_thermal_calib_info & table, double hum_temp )
{

    // curr temp is under minimum
    if( hum_temp <= table.md.min_temp )
        return 1 / (double)table.vals[0].scale;

    // curr temp is above maximum
    if( hum_temp >= table.md.max_temp )
        return 1 / (double)table.vals[rgb_thermal_calib_info::resolution - 1].scale;

    auto temp_range = table.md.max_temp - table.md.min_temp;
    auto temp_interval = temp_range / ( rgb_thermal_calib_info::resolution + 1 );

    float i = temp_interval;
    for( int ind = 0; ind < rgb_thermal_calib_info::resolution; ind++ )
    {
        if( hum_temp <= i )
        {
            return 1 / (double)table.vals[ind].scale;
        }
        i += temp_interval;
    }
    throw std::runtime_error( librealsense::to_string() << hum_temp << "is not valid " );
}

static std::pair < double, double > correct_thermal_scale( std::pair< double, double > in_calib,
                       double scale )
{
    return { in_calib.first * scale, in_calib.second * scale };
} 

}  // namespace l500
}  // namespace thermal_loop
}  // namespace algo
}  // namespace librealsense
