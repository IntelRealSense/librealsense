//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once
#include "l500-thermal-loop.h"

namespace librealsense {
namespace algo {
namespace thermal_loop {
namespace l500 {


using calib_info = l500_thermal_loop::rgb_thermal_calib_info;

calib_info l500_thermal_loop::parse_thermal_table( const std::vector< byte > & data )
{
    rgb_thermal_calib_info res;
    std::vector< float > float_vac;

    float_vac.assign( (float *)data.data(),
                      (float *)data.data() + ( data.size() / sizeof( float ) ) );

    res.md.min_temp = float_vac[0];
    res.md.max_temp = float_vac[1];

    const int meta_data_size = sizeof( calib_info::table_meta_data ) / sizeof( float );
    const int table_data_size = sizeof( calib_info::table_data ) / sizeof( float );

    if( ( float_vac.size() - meta_data_size ) / table_data_size == calib_info::resolution )
    {
        res.md.reference_temp = float_vac[2];
        res.md.valid = float_vac[3];
        res.vals.assign( (calib_info::table_data *)( float_vac.data() + meta_data_size ),
                         (calib_info::table_data *)float_vac.data() + calib_info::resolution + 1 );
    }
    else
    {
        throw std::runtime_error( librealsense::to_string() << "file size (" << data.size()
                                                            << ") does not match data size " );
    }

    return res;
}

double l500_thermal_loop::get_rgb_current_thermal_scale( const rgb_thermal_calib_info & table,
                                                         double hum_temp )
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

fx_fy l500_thermal_loop::correct_thermal_scale( std::pair< double, double > in_calib,
                                                   double scale )
{
    return { in_calib.first * scale, in_calib.second * scale };
}
}
}
}
}
