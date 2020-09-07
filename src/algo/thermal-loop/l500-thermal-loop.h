// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>


namespace librealsense {
namespace algo {
namespace thermal_loop {

static const int num_of_records = 29;

struct table_meta_data
{
    float min_temp;
    float max_temp;
    float reference_temp;
    float valid;
};

struct table_data
{
    float x_y_scale;
    float p[3];  // parameters which effects offset that are not in use
};

struct thermal_table_data
{
    table_meta_data md;
    std::vector< table_data > vals;
};

static thermal_table_data parse_thermal_table( std::vector< byte > data )
{
    thermal_table_data res;
    std::vector< float > float_vac;
    float_vac.assign( (float *)data.data(),
                      (float *)data.data() + ( data.size() / sizeof( float ) ) );

    res.md.min_temp = float_vac[0];
    res.md.max_temp = float_vac[1];

    const int meta_data_size = sizeof( table_meta_data ) / sizeof( float );
    const int table_data_size = sizeof( table_data ) / sizeof( float );
    if( ( float_vac.size() - meta_data_size ) / table_data_size == num_of_records )  // new format
    {
        res.md.reference_temp = float_vac[2];
        res.md.valid = float_vac[3];
        res.vals.assign( (table_data *)( float_vac.data() + meta_data_size ),
                         (table_data *)float_vac.data() + num_of_records );
    }
    else
    {
        throw std::runtime_error( librealsense::to_string() << "file size (" << data.size()
                                                            << ") does not match data size " );
    }

    return res;
}

static double get_rgb_current_thermal_scale( const thermal_table_data & table, double hum_temp )
{

    if( hum_temp <= table.md.min_temp )
        return 1 / (double)table.vals[0].x_y_scale;

    if( hum_temp >= table.md.max_temp )
        return 1 / (double)table.vals[num_of_records - 1].x_y_scale;

    auto temp_range = table.md.max_temp - table.md.min_temp;
    auto temp_step = temp_range / ( num_of_records + 1 );

    float i = temp_step;
    for( int ind = 0; ind < num_of_records; ind++ )
    {
        if( hum_temp <= i )
        {
            return 1 / (double)table.vals[ind].x_y_scale;
        }
        i += temp_step;
    }
    throw std::runtime_error( librealsense::to_string() << hum_temp << "is not valid " );
}

static std::pair < double, double > correct_thermal_scale( std::pair< double, double > in_calib,
                       double scale )
{
    return { in_calib.first * scale, in_calib.second * scale };
} 
}  // namespace thermal_loop
}  // namespace algo
}  // namespace librealsense
