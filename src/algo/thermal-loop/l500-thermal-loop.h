// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>
#include "../../types.h"

namespace librealsense {
namespace algo {
namespace thermal_loop {
namespace l500 {

using fx_fy = std::pair< double, double >;

#pragma pack( push, 1 )

class l500_thermal_loop
{
public:
    struct rgb_thermal_calib_info
    {
        static const int table_id = 0x317;
        static const int resolution = 29;

        struct table_meta_data
        {
            float min_temp;
            float max_temp;
            float reference_temp;  // not used
            float valid;           // not used
        };

        struct table_data
        {
            float scale;
            float p[3];  // parameters which effects offset that are not in use
        };

        table_meta_data md;
        std::vector< table_data > vals;

        std::vector< byte > get_raw_data()
        {
            std::vector< byte > res;
            std::vector< float > data;
            data.push_back( md.min_temp );
            data.push_back( md.max_temp );
            data.push_back( md.reference_temp );
            data.push_back( md.valid );

            for( auto i = 0; i < vals.size(); i++ )
            {
                data.push_back( vals[i].scale );
                data.push_back( vals[i].p[0] );
                data.push_back( vals[i].p[1] );
                data.push_back( vals[i].p[2] );
            }

            res.assign( (byte *)( data.data() ), (byte *)( data.data() + data.size() ) );
            return res;
        }
    };
#pragma pack( pop )

    static rgb_thermal_calib_info parse_thermal_table( const std::vector< byte > & data );

    static double get_rgb_current_thermal_scale( const rgb_thermal_calib_info & table,
                                                 double hum_temp );

    static fx_fy correct_thermal_scale( std::pair< double, double > in_calib,
                                                              double scale );
};
// this struct based on RGB_Thermal_Info_CalibInfo table


}  // namespace l500
}  // namespace thermal_loop
}  // namespace algo
}  // namespace librealsense
