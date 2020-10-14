// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "l500-thermal-loop.h"
#include "../../l500/l500-private.h"

namespace librealsense {
namespace algo {
namespace thermal_loop {
namespace l500 {


const int thermal_calibration_table::id = 0x317;

thermal_calibration_table::thermal_calibration_table() : _resolution(0)
{
    _header.max_temp = 0;
    _header.min_temp = 0;
    _header.reference_temp = 0;
    _header.valid = 0;
}

thermal_calibration_table::thermal_calibration_table( const std::vector< byte > & data,
                                                      int resolution )
    : _resolution( resolution )
{
    auto expected_size = sizeof( thermal_table_header )
                       + sizeof( thermal_bin ) * resolution;

    _header.valid = 0;

    if( data.size() != expected_size )
        throw std::runtime_error( librealsense::to_string()
                                  << "data size (" << data.size()
                                  << ") does not meet expected size " << expected_size );

    _header = *(thermal_table_header *)data.data();
    
    // The table may be invalid if the unit has not gone thru calibration
    if( ! _header.valid )
        throw std::runtime_error( "thermal calibration table is not valid" );

    auto data_ptr = (thermal_bin *)( data.data() + sizeof( thermal_table_header ));
    bins.assign( data_ptr, data_ptr + resolution );
}


bool operator==( const thermal_calibration_table & lhs, const thermal_calibration_table & rhs )
{
    if( lhs.bins.size() != rhs.bins.size() )
        return false;

    if( lhs._header.max_temp != rhs._header.max_temp || lhs._header.min_temp != rhs._header.min_temp
        || lhs._header.reference_temp != rhs._header.reference_temp
        || lhs._header.valid != rhs._header.valid )
        return false;

    for( auto i = 0; i < rhs.bins.size(); i++ )
    {
        if( lhs.bins[i].scale != rhs.bins[i].scale || lhs.bins[i].sheer != rhs.bins[i].sheer
            || lhs.bins[i].tx != rhs.bins[i].tx || lhs.bins[i].ty != rhs.bins[i].ty )
            return false;
    }
    return true;
}


double thermal_calibration_table::get_thermal_scale( double hum_temp ) const
{
    auto scale = bins[_resolution - 1].scale;

    auto temp_range = _header.max_temp - _header.min_temp;
    // there are 29 bins between min and max temps so 30 equal intervals
    auto const interval = temp_range / ( _resolution + 1 );
    // T:   |---|---|---| ... |---|
    //    min   0   1   2 ... 28  max
    size_t index = 0;
    for( double temp = _header.min_temp; index < _resolution;
            ++index, temp += interval )
    {
        auto interval_max = temp + interval;
        if( hum_temp <= interval_max )
        {
            scale = bins[index].scale;
            break;
        }
    }

    // The "scale" is meant to be divided by, but we want something to multiply with!
    if( scale == 0 )
        throw std::runtime_error( "invalid 0 scale in thermal table" );
    return 1. / scale;
}


std::vector< byte > thermal_calibration_table::build_raw_data() const
{
    std::vector< float > data;

    data.push_back( _header.min_temp );
    data.push_back( _header.max_temp );
    data.push_back( _header.reference_temp );
    data.push_back( _header.valid );

    for( auto i = 0; i < bins.size(); i++ )
    {
        data.push_back( bins[i].scale );
        data.push_back( bins[i].sheer );
        data.push_back( bins[i].tx );
        data.push_back( bins[i].ty );
    }

    std::vector< byte > res;
    res.assign( (byte *)( data.data() ), (byte *)( data.data() + data.size() ) );
    return res;
}


}
}
}
}
