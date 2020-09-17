// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "l500-thermal-loop.h"
#include "../../l500/l500-private.h"

namespace librealsense {
namespace algo {
namespace thermal_loop {
namespace l500 {


thermal_calibration_table::thermal_calibration_table( const std::vector< byte > & data ) 
{
    float const * header_ptr = (float *)( data.data() + sizeof( ivcam2::table_header ) );

    auto expected_size = sizeof( ivcam2::table_header ) + sizeof( thermal_table_header )
                       + sizeof( temp_data ) * resolution;

    if( data.size() != expected_size )
        throw std::runtime_error( librealsense::to_string()
                                  << "data size (" << data.size()
                                  << ") does not meet expected size " << expected_size );

    header = *(thermal_table_header *)( header_ptr );

    auto data_ptr = (temp_data *)( data.data() + sizeof( ivcam2::table_header )
                                   + sizeof( thermal_table_header ) );
    vals.assign( data_ptr, data_ptr + resolution );
}

double thermal_calibration_table::get_current_thermal_scale( const double & hum_temp ) const
{
    auto scale = vals[resolution - 1].scale;

    // curr temp is under minimum
    if( hum_temp <= header.min_temp )
    {
        scale = vals[0].scale;
    }
    else
    {
        auto temp_range = header.max_temp - header.min_temp;
        // there are 29 bins between min and max temps so its divides to 30 equals intervals
        auto interval = temp_range / ( thermal_calibration_table::resolution + 1 );

        for( double temp = header.min_temp, index = 0; index < resolution;
             ++index, temp += interval )
        {
            auto interval_max = temp + interval;
            if (hum_temp <= interval_max)
            {
                scale = vals[index].scale;
                break;
            }
        }
    }

    if( scale == 0 )
        throw std::runtime_error( "Scale value in index 0 is 0 " );
    return 1. / scale;
}

std::vector< byte > thermal_calibration_table::build_raw_data() const
{
    std::vector< float > data;
    data.resize( sizeof( ivcam2::table_header ) / sizeof(float) );
    data.push_back( header.min_temp );
    data.push_back( header.max_temp );
    data.push_back( header.reference_temp );
    data.push_back( header.valid );

    for( auto i = 0; i < vals.size(); i++ )
    {
        data.push_back( vals[i].scale );
        data.push_back( vals[i].sheer );
        data.push_back( vals[i].tx );
        data.push_back( vals[i].ty );
    }

    std::vector< byte > res;
    res.assign( (byte *)( data.data() ), (byte *)( data.data() + data.size() ) );
    return res;
}

}
}
}
}
