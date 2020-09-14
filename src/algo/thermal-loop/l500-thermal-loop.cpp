// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "l500-thermal-loop.h"

namespace librealsense {
namespace algo {
namespace thermal_loop {
namespace l500 {


thermal_calibration_table thermal_calibration_table::parse_thermal_table( const std::vector< byte > & data )
{
    thermal_calibration_table res;
    float const * float_data = (float *)data.data();
  

    const int meta_data_size
        = sizeof( thermal_calibration_table::table_meta_data ) / sizeof( float );
    const int temp_data_size = sizeof( thermal_calibration_table::temp_data ) / sizeof( float );

    if( ( data.size() / sizeof(float) - meta_data_size ) / temp_data_size
        != thermal_calibration_table::resolution )
        throw std::runtime_error( librealsense::to_string() << "data size (" << data.size()
                                                            << ") does not expected size " );


    res.md = *(thermal_calibration_table::table_meta_data *)( &float_data[0] );

    res.vals.assign( (thermal_calibration_table::temp_data *)( &float_data[meta_data_size] ),
                     (thermal_calibration_table::temp_data *)( &float_data[meta_data_size] )
                         + thermal_calibration_table::resolution );

    return res;
}

double
thermal_calibration_table::get_current_thermal_scale(double hum_temp )
{
    // curr temp is under minimum
    if (hum_temp <= md.min_temp)
    {
        if( vals[0].scale == 0)
            throw std::runtime_error( "Scale value in index 0 is 0 " );

        return 1. / vals[0].scale;
    }

    // curr temp is above maximum
    if (hum_temp >= md.max_temp)
    {
        if( vals[thermal_calibration_table::resolution - 1].scale == 0 )
            throw std::runtime_error( to_string()
                                      << "Scale value in index "
                                      << thermal_calibration_table::resolution - 1 << " is 0 " );

        return 1. / vals[thermal_calibration_table::resolution - 1].scale;
    }
       

    auto temp_range = md.max_temp - md.min_temp;
    // there are 29 bins between min and max temps so its divides to 30 equals intervals
    auto temp_interval = temp_range / ( thermal_calibration_table::resolution + 1 );

    for( double temp = temp_interval, ind = 0;
         temp <= md.max_temp, ind < thermal_calibration_table::resolution;
         temp += temp_interval, ind++ )
    {
        if( hum_temp <= temp && ind < thermal_calibration_table::resolution )
        {
            if( vals[(int)ind].scale == 0 )
                throw std::runtime_error( to_string()
                                          << "Scale value in index " << ind << " is 0 " );

            return 1. / vals[ind].scale;
        }
    }

     // the loop does not cover the last range [resolution-max_temp]
    if( hum_temp < md.max_temp )
    {
        if( vals[thermal_calibration_table::resolution - 1].scale == 0 )
            throw std::runtime_error( to_string()
                                      << "Scale value in index "
                                      << thermal_calibration_table::resolution - 1 << " is 0 " );

        return 1. / vals[thermal_calibration_table::resolution - 1].scale;
    }
    throw std::runtime_error( librealsense::to_string() << hum_temp << " is not valid " );
}

fx_fy correct_thermal_scale( std::pair< double, double > in_calib,
                                                   double scale )
{
    return { in_calib.first * scale, in_calib.second * scale };
}
}
}
}
}
